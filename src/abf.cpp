/* abf Brainfuck-interpreter.
 * This interpreter is ``nice.''
 * Copyright Troels Henriksen 2004-2005.
 * Released under the terms of the GPL-2 or later.
 */
#include <string>
#include <vector>
#include <algorithm>
#include <stdexcept>

#define EXTENSIVE
#ifdef EXTENSIVE
#define BRAINTWIST
#define BRAINFORK
#endif

using byte = unsigned char;
using datapair = std::pair<std::vector<byte>*, int>;
using context = std::pair<datapair, datapair>;

// Utility stuff.
static inline char get_token(const context& c) { return (*c.first.first)[c.first.second]; }
static inline void next_token(context& c) { c.first.second++; }
static inline void prev_token(context& c) { c.first.second--; }
static inline bool first_token(const context& c) { return c.first.second == 0; }
static inline bool last_token(const context& c) { return c.first.second == static_cast<int>(c.first.first->size()); }
static inline bool can_get_token(const context& c) { return c.first.second <= static_cast<int>(c.first.first->size()); }
static inline byte get_byte(const context& c) { return (*c.second.first)[c.second.second]; }
static inline void next_byte(context& c) { c.second.second++; }
static inline void prev_byte(context& c) { c.second.second--; }
static inline void incr_byte(context& c) { (*c.second.first)[c.second.second]++; }
static inline void decr_byte(context& c) { (*c.second.first)[c.second.second]--; }
static inline void set_byte(context& c, byte val) { (*c.second.first)[c.second.second] = val; }

char* bf_exec(const std::string& bfcode) {
	std::vector<byte> data(100000, 0);
	std::vector<byte> code(bfcode.begin(), bfcode.end());
	code.resize(code.size() + 100000, 0);

	char* out = new char[1000000];
	unsigned int outpos = 0;

	std::vector<context> ctx;
	ctx.push_back({ { &code, 0 }, { &data, 0 } });

	while (!ctx.empty()) {
		for (int it = 0; it < static_cast<int>(ctx.size()); ++it) {
			if (!can_get_token(ctx[it])) continue;

			switch (get_token(ctx[it])) {
			case '+': incr_byte(ctx[it]); break;
			case '-': decr_byte(ctx[it]); break;
			case '.': out[outpos++] = static_cast<char>(get_byte(ctx[it])); break;
			case ',': {
				set_byte(ctx[it], 0);
				break;
			}
			case '>': next_byte(ctx[it]); break;
			case '<': prev_byte(ctx[it]); break;
			case '[':
				if (get_byte(ctx[it]) == 0) {
					int brack = 0;
					next_token(ctx[it]);
					while (brack > 0 || get_token(ctx[it]) != ']') {
						if (get_token(ctx[it]) == '[') brack++;
						if (get_token(ctx[it]) == ']') brack--;
						next_token(ctx[it]);
						if (last_token(ctx[it]))
							throw std::logic_error("Unbalanced brackets: Missing ']'");
					}
				}
				break;
			case ']':
				if (get_byte(ctx[it]) != 0) {
					prev_token(ctx[it]);
					int brack = 0;
					while (brack > 0 || get_token(ctx[it]) != '[') {
						if (first_token(ctx[it]) && brack > 0)
							throw std::logic_error("Unbalanced brackets: Missing '['");
						if (get_token(ctx[it]) == ']') brack++;
						if (get_token(ctx[it]) == '[') brack--;
						prev_token(ctx[it]);
					}
				}
				break;
#ifdef BRAINTWIST
			case 'X': {
				datapair tmp = ctx[it].first;
				ctx[it].first = ctx[it].second;
				ctx[it].second = tmp;
				break;
			}
#endif
#ifdef BRAINFORK
			case 'Y': {
				byte tmp_val = get_byte(ctx[it]);
				next_byte(ctx[it]);
				set_byte(ctx[it], tmp_val + 1);
				prev_byte(ctx[it]);
				set_byte(ctx[it], 0);
				ctx.push_back({ { ctx[it].first.first,  ctx[it].first.second + 1 }, { ctx[it].second.first, ctx[it].second.second + 1 } });
				break;
			}
#endif
			default:
				break;
			}
			next_token(ctx[it]);
		}

		ctx.erase(
			std::remove_if(ctx.begin(), ctx.end(), [](const context& c) {
				return !can_get_token(c);
			}), ctx.end());
	}

	out[outpos] = '\0';
	return out;
}