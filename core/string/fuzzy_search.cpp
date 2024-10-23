/**************************************************************************/
/*  fuzzy_search.cpp                                                      */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "fuzzy_search.h"

#include "core/string/ustring.h"
#include "core/variant/variant.h"

const float cull_factor = 0.1f;
const float cull_cutoff = 30.0f;
const String boundary_chars = "/\\-_.";

bool is_valid_interval(const Vector2i &p_interval) {
	// Empty intervals are represented as (-1, -1).
	return p_interval.x >= 0 && p_interval.y >= p_interval.x;
}

Vector2i extend_interval(const Vector2i &p_a, const Vector2i &p_b) {
	if (!is_valid_interval(p_a)) {
		return p_b;
	}
	if (!is_valid_interval(p_b)) {
		return p_a;
	}
	return Vector2i(MIN(p_a.x, p_b.x), MAX(p_a.y, p_b.y));
}

bool is_word_boundary(const String &str, int index) {
	if (index == -1 || index == str.size()) {
		return true;
	}
	return boundary_chars.find_char(str[index]) != -1;
}

void FuzzyTokenMatch::add_substring(int substring_start, int substring_length) {
	substrings.append(Vector2i(substring_start, substring_length));
	matched_length += substring_length;
	Vector2i substring_interval = { substring_start, substring_start + substring_length - 1 };
	interval = extend_interval(interval, substring_interval);
}

bool FuzzyTokenMatch::intersects(const Vector2i &other_interval) const {
	if (!is_valid_interval(interval) || !is_valid_interval(other_interval)) {
		return false;
	}
	return interval.y >= other_interval.x && interval.x <= other_interval.y;
}

bool FuzzySearchResult::can_add_token_match(const FuzzyTokenMatch &p_match) const {
	if (p_match.get_miss_count() > miss_budget) {
		return false;
	}

	if (p_match.intersects(match_interval)) {
		if (token_matches.size() == 1) {
			return false;
		}
		for (const FuzzyTokenMatch &existing_match : token_matches) {
			if (existing_match.intersects(p_match.interval)) {
				return false;
			}
		}
	}

	return true;
}

bool FuzzyTokenMatch::is_case_insensitive(const String &p_original, const String &p_adjusted) const {
	for (const Vector2i &substr : substrings) {
		const int end = substr.x + substr.y;
		for (int i = substr.x; i < end; i++) {
			if (p_original[i] != p_adjusted[i]) {
				return true;
			}
		}
	}
	return false;
}

void FuzzySearchResult::score_token_match(FuzzyTokenMatch &p_match, bool p_case_insensitive) const {
	// This can always be tweaked more. The intuition is that exact matches should almost always
	// be prioritized over broken up matches, and other criteria more or less act as tie breakers.

	p_match.score = -20 * p_match.get_miss_count() - (p_case_insensitive ? 3 : 0);

	for (const Vector2i &substring : p_match.substrings) {
		// Score longer substrings higher than short substrings
		int substring_score = substring.y * substring.y;
		// Score matches deeper in path higher than shallower matches
		if (substring.x > dir_index) {
			substring_score *= 2;
		}
		// Score matches on a word boundary higher than matches within a word
		if (is_word_boundary(target, substring.x - 1) || is_word_boundary(target, substring.x + substring.y)) {
			substring_score += 4;
		}
		// Score exact query matches higher than non-compact subsequence matches
		if (substring.y == p_match.token_length) {
			substring_score += 100;
		}
		p_match.score += substring_score;
	}
}

void FuzzySearchResult::add_token_match(const FuzzyTokenMatch &p_match) {
	score += p_match.score;
	match_interval = extend_interval(match_interval, p_match.interval);
	miss_budget -= p_match.get_miss_count();
	token_matches.append(p_match);
}

void remove_low_scores(Vector<FuzzySearchResult> &p_results, const float p_cull_score) {
	// Removes all results with score < p_cull_score in-place.
	int i = 0;
	int j = p_results.size() - 1;
	FuzzySearchResult *results = p_results.ptrw();

	while (true) {
		// Advances i to an element to remove and j to an element to keep
		while (j >= i && results[j].score < p_cull_score) j--;
		while (i < j && results[i].score >= p_cull_score) i++;
		if (i >= j) break;
		results[i++] = results[j--];
	}

	p_results.resize(j + 1);
}

void FuzzySearch::sort_and_filter(Vector<FuzzySearchResult> &p_results) const {
	if (p_results.is_empty()) {
		return;
	}

	float avg_score = 0;
	float max_score = 0;

	for (const FuzzySearchResult &result : p_results) {
		avg_score += result.score;
		max_score = MAX(max_score, result.score);
	}

	// TODO: Tune scoring and culling here to display fewer subsequence soup matches when good matches
	//  are available.
	avg_score /= p_results.size();
	float cull_score = MIN(cull_cutoff, Math::lerp(avg_score, max_score, cull_factor));
	remove_low_scores(p_results, cull_score);

	struct FuzzySearchResultComparator {
		bool operator()(const FuzzySearchResult &A, const FuzzySearchResult &B) const {
			// Sort on (score, length, alphanumeric) to ensure consistent ordering
			if (A.score == B.score) {
				if (A.target.length() == B.target.length()) {
					return A.target < B.target;
				}
				return A.target.length() < B.target.length();
			}
			return A.score > B.score;
		}
	};

	SortArray<FuzzySearchResult, FuzzySearchResultComparator> sorter;

	if (p_results.size() > max_results) {
		sorter.partial_sort(0, p_results.size(), max_results, p_results.ptrw());
		p_results.resize(max_results);
	} else {
		sorter.sort(p_results.ptrw(), p_results.size());
	}
}

bool FuzzySearch::try_match_token(
		FuzzyTokenMatch &p_match,
		const String &p_token,
		const String &p_target,
		int p_offset,
		int p_miss_budget) const {
	p_match.token_length = p_token.length();
	int run_start = -1;
	int run_len = 0;

	if (!allow_subsequences) {
		int idx = p_target.find(p_token, p_offset);
		if (idx == -1) {
			return false;
		}
		p_match.add_substring(idx, p_token.length());
		return true;
	}

	// Search for the subsequence p_token in p_target starting from p_offset, recording each substring for
	// later scoring and display.
	for (int i = 0; i < p_token.length(); i++) {
		int new_offset = p_target.find_char(p_token[i], p_offset);
		if (new_offset < 0) {
			if (--p_miss_budget < 0) {
				return false;
			}
		} else {
			if (run_start == -1 || p_offset != new_offset) {
				if (run_start != -1) {
					p_match.add_substring(run_start, run_len);
				}
				run_start = new_offset;
				run_len = 1;
			} else {
				run_len += 1;
			}
			p_offset = new_offset + 1;
		}
	}

	if (run_start != -1) {
		p_match.add_substring(run_start, run_len);
	}

	return true;
}

void FuzzySearch::set_query(const String &p_query) {
	tokens = p_query.split(" ", false);
	case_sensitive = !p_query.is_lowercase();

	struct TokenComparator {
		bool operator()(const String &A, const String &B) const {
			if (A.length() == B.length()) {
				return A < B;
			}
			return A.length() > B.length();
		}
	};

	// Prioritize matching longer tokens before shorter ones since match overlaps are not accepted
	tokens.sort_custom<TokenComparator>();
}

bool FuzzySearch::search(const String &p_target, FuzzySearchResult& p_result) const {
	p_result.target = p_target;
	p_result.dir_index = p_target.rfind_char('/');
	p_result.miss_budget = max_misses;

	String adjusted_target = case_sensitive ? p_target : p_target.to_lower();

	// For each token, eagerly generate subsequences starting from index 0 and keep the best scoring one
	// which does not conflict with prior token matches. This is not ensured to find the highest scoring
	// combination of matches, or necessarily the highest scoring single subsequence, as it only considers
	// eager subsequences for a given index, and likewise eagerly finds matches for each token in sequence.
	for (const String &token : tokens) {
		FuzzyTokenMatch best_match;
		int offset = start_offset;

		while (true) {
			FuzzyTokenMatch match;
			if (!try_match_token(match, token, adjusted_target, offset, p_result.miss_budget)) {
				break;
			}
			if (p_result.can_add_token_match(match)) {
				p_result.score_token_match(match, match.is_case_insensitive(p_target, adjusted_target));
				if (best_match.token_length == 0 || best_match.score < match.score) {
					best_match = match;
				}
			}
			if (is_valid_interval(match.interval)) {
				offset = match.interval.x + 1;
			} else {
				break;
			}
		}

		if (best_match.token_length == 0) {
			return false;
		}

		p_result.add_token_match(best_match);
	}

	return true;
}

void FuzzySearch::search_all(const PackedStringArray &p_targets, Vector<FuzzySearchResult> &p_results) const {
	p_results.clear();

	for (const String &target : p_targets) {
		FuzzySearchResult result;
		if (search(target, result)) {
			p_results.append(result);
		}
	}

	sort_and_filter(p_results);
}
