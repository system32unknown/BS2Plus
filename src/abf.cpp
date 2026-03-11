/* abf Brainfuck-interpreter.
 * This interpreter is ``nice.''
 * Copyright Troels Henriksen 2004-2005.
 * Released under the terms of the GPL-2 or later.
 */

#include <iostream>
#include <string>
#include <fstream>
#include <stdexcept>
#include <vector>
#include <algorithm>
#include <functional>

#define EXTENSIVE // Extend nice Brainfuck.

#ifdef EXTENSIVE
// Braintwist is a Brainfuck-derivative that defines a X-operator that swaps
// the data and code arrays.
#define BRAINTWIST
// Brainfork provides threads for Brainfuck. The term "context" is, somewhat
// confusingly, used to refer to a pointer-pair used to implement the threads.
#define BRAINFORK
#endif

using namespace std;

typedef unsigned char byte;
typedef pair<vector<unsigned char> *, int> datapair;

// Utility stuff.

inline char get_token(const pair<datapair, datapair> &dpp)
{
  // cout << dpp.first.second << endl;
  return (*(dpp.first.first))[dpp.first.second];
}

inline void next_token(pair<datapair, datapair> &dpp)
{
  dpp.first.second++;
}

inline void prev_token(pair<datapair, datapair> &dpp)
{
  dpp.first.second--;
}

inline bool first_token(const pair<datapair, datapair> &dpp)
{
  return dpp.first.second == 0;
}

inline bool last_token(const pair<datapair, datapair> &dpp)
{
  return dpp.first.second == (signed)dpp.first.first->size();
}

inline bool can_get_token(const pair<datapair, datapair> &dpp)
{
  return dpp.first.second <= (signed)dpp.first.first->size();
}

inline char get_byte(const pair<datapair, datapair> &dpp)
{
  return (*(dpp.second.first))[dpp.second.second];
}

inline void next_byte(pair<datapair, datapair> &dpp)
{
  dpp.second.second++;
}

inline void prev_byte(pair<datapair, datapair> &dpp)
{
  dpp.second.second--;
}

inline void incr_byte(pair<datapair, datapair> &dpp)
{
  ((*(dpp.second.first))[dpp.second.second])++;
}

inline void decr_byte(pair<datapair, datapair> &dpp)
{
  ((*(dpp.second.first))[dpp.second.second])--;
}

inline void set_byte(pair<datapair, datapair> &dpp, unsigned char val)
{
  (*(dpp.second.first))[dpp.second.second] = val;
}

class stale_context_remover
{
public:
  stale_context_remover(int code_size) : m_code_size(code_size)
  {
  }

  bool operator()(const pair<datapair, datapair> &x)
  {
    return !can_get_token(x);
  }

private:
  const int m_code_size;
};

char *bf_exec(const std::string &code);
std::string read_bf(std::istream &codestrm);

/*char *read_bf(istream& codestrm)
{
  string codestr = "";

  for (string tmp; getline(codestrm,tmp);)
    codestr += tmp;

  return codestr;
}*/

char *bf_exec(const string &bfcode)
{
  vector<unsigned char> data(100000);
  vector<unsigned char> code;
  char *out = new char[1000000];
  unsigned int outpos = 0;
  copy(bfcode.begin(), bfcode.end(), back_inserter(code));
  code.resize(code.size() + 100000, 0);

  int code_length = code.size() - 100000; // May be invalidated (set to 0xFFFF).

  long int comment_count = 0;

  // ((Code* . Codeptr) . (Data* . Dataptr)).
  vector<pair<datapair, datapair>> context_vec;
  context_vec.push_back(make_pair(make_pair(&code, 0),
                                  make_pair(&data, 0))); // Default context.

  while (context_vec.size() > 0)
  {
    for (int it = 0;
         it != (signed)context_vec.size();
         ++it)
    {
      if (can_get_token(context_vec[it]))
      {
        switch (get_token(context_vec[it]))
        {
        case '+':
          incr_byte(context_vec[it]);
          break;
        case '-':
          decr_byte(context_vec[it]);
          break;
        case '.':
          // cout << (int)get_byte(context_vec[it]);
          out[outpos++] = get_byte(context_vec[it]);
          break;
        case ',':
          unsigned char tmpbyte;
          cin >> tmpbyte;
          if (cin.eof())
          {
            tmpbyte = 0;
            // Clear stream.
            cin.clear();
            cin >> tmpbyte;
          }
          set_byte(context_vec[it], tmpbyte);
          break;
        case '>':
          next_byte(context_vec[it]);
          break;
        case '<':
          prev_byte(context_vec[it]);
          break;
        case '[':
          if (get_byte(context_vec[it]) == 0)
          {
            int brack = 0;
            next_token(context_vec[it]);
            while (brack > 0 || get_token(context_vec[it]) != ']')
            {
              if (get_token(context_vec[it]) == '[')
                brack++;
              if (get_token(context_vec[it]) == ']')
                brack--;
              next_token(context_vec[it]);
              // Check for unbalances:
              if (last_token(context_vec[it]))
                throw logic_error("Unbalanced parantheses: Missing ']'");
            }
          }
          break;
        case ']':
        {
          if (get_byte(context_vec[it]) != 0)
          {
            prev_token(context_vec[it]);
            int brack = 0;
            while (brack > 0 || get_token(context_vec[it]) != '[')
            {
              if (first_token(context_vec[it]) && brack > 0)
                throw logic_error("Unbalanced parantheses: Missing '['");
              if (get_token(context_vec[it]) == ']')
                brack++;
              if (get_token(context_vec[it]) == '[')
                brack--;
              prev_token(context_vec[it]);
            }
          }
        }
        break;
        // Extensions to standard Brainfuck.
#ifdef EXTENSIVE
#ifdef BRAINTWIST
        case 'X':
        {
          // Inexpensive operation (pointer swap).
          datapair tmp = context_vec[it].first;
          context_vec[it].first = context_vec[it].second;
          context_vec[it].second = tmp;

          // Basically, the user may do just about everything now.
          // Therefore, we cannot assume that the code is of any particular size.
          code_length = 0xFFFF;
        }
        break;
#endif
#ifdef BRAINFORK
        case 'Y':
        {
          unsigned char tmp_val = get_byte(context_vec[it]);
          next_byte(context_vec[it]);
          set_byte(context_vec[it], tmp_val + 1);
          prev_byte(context_vec[it]);
          set_byte(context_vec[it], 0);
          context_vec.push_back(make_pair(make_pair(context_vec[it].first.first,
                                                    context_vec[it].first.second + 1),
                                          make_pair(context_vec[it].second.first,
                                                    context_vec[it].second.second + 1)));
        }
        break;
#endif
#endif
        default:
          comment_count++; // Just for stat.
          break;
        }
        next_token(context_vec[it]);
      }
    }
    // Remove ended contexts.
    context_vec.erase(remove_if(context_vec.begin(),
                                context_vec.end(),
                                stale_context_remover(0)),
                      context_vec.end());
  }
  out[outpos++] = 0;
  return out;
}