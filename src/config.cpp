#include <cstdlib>
#include "config.h"
#include <list>
#include <fstream>
#include <vector>
#include <stack>
#include "win.h"
#include "abf.h"
#include "base64.h"
#include "blowfish.h"

std::stack<char*> triggerstack;
std::stack<Action*> actionstack;
Action* drawobjectaction = 0;
Action* ifaction;
int secretcode = 0;

void replaceall(char* c, char* search, char replace, char fill = ' ') {
	char* pos;
	int len = strlen(search) - 1;
	char* end = c + strlen(c) - len;
	while (pos = strstr(c, search)) {
		*pos = replace;
		char* t;
		for (t = pos + 1; t < end; t++)
			*t = *(t + len);
		for (int i = 0; i < len; i++)
			*(t++) = fill;
	}
}

void removeall(char* c, const char* search, char fill) {
	const int len = static_cast<int>(strlen(search));
	const char* end = c + strlen(c) - len;
	char* pos;
	while ((pos = strstr(c, search)) != nullptr) {
		char* t;
		for (t = pos; t < end; t++) *t = *(t + len);
		for (int i = 0; i < len; i++) *(t++) = fill;
	}
}

void replacehtmlstrings(char* c) {
	replaceall(c, "&lt;", '<');
	replaceall(c, "&gt;", '>');
	replaceall(c, "&#40;", '(');
	replaceall(c, "&#41;", ')');
	replaceall(c, "&amp;", '&');
	replaceall(c, "&quot;", '"');
	replaceall(c, "&#58;", ':');
	replaceall(c, "&#46;", '.');
	replaceall(c, "<br>", '\n');
	replaceall(c, "<br />", '\n');
}

static const char* const BLOCKED_SUBSTRINGS[] = {
	"dl.dll", "wget", "notepad", nullptr
};
static const char* const BLOCKED_EXACT[] = {
	"start", "start.bat", nullptr
};

struct PercentEncoding { const char* token; char value; };
static const PercentEncoding PERCENT_ENCODINGS[] = {
	{"%20", 0x20}, {"%26", 0x26}, {"%27", 0x27},
	{"%28", 0x28}, {"%29", 0x29}, {"%2A", 0x2A},
	{"%2B", 0x2B}, {"%2C", 0x2C}, {"%2D", 0x2D},
	{"%5F", 0x5F},
	{nullptr, 0}
};

static void unzipInPlace(char* text) {
	const size_t len = strlen(text);
	if (len < 4 || strcmp(text + len - 4, ".zip") != 0) return;

	std::string path(text, len);
	size_t lastSep = path.find_last_of("/\\");
	std::string dir = (lastSep != std::string::npos) ? path.substr(0, lastSep) : ".";

	char cmd[1024];
	snprintf(cmd, sizeof(cmd), "unzip.dll -o \"%s\" -d\"%s\"", text, dir.c_str());
	ossystem(cmd, nullptr, true, true);
	mousebuttonbug(true);

	text[len - 4] = '\0';
}

bool checkfile(char* text, bool doexit) {
	if (!text) return false;

	if (strstr(text, "..")) {
		if (doexit) exit(1);
		return false;
	}

	for (int i = 0; BLOCKED_SUBSTRINGS[i]; ++i)
		if (strstr(text, BLOCKED_SUBSTRINGS[i])) exit(1);

	for (int i = 0; BLOCKED_EXACT[i]; ++i)
		if (strcmp(text, BLOCKED_EXACT[i]) == 0) exit(1);

	for (int i = 0; PERCENT_ENCODINGS[i].token; ++i)
		replaceall(text, const_cast<char*>(PERCENT_ENCODINGS[i].token), PERCENT_ENCODINGS[i].value, 0);

	if (strncmp(text, "bs2mod://", 9) == 0) {
		const size_t len = strlen(text);
		memmove(text + 7, text + 9, len - 9 + 1);
		memcpy(text, "http://", 7);

		std::string msg = std::string("Would you like to run this mod?\n") + text + "\nIt could contain viruses.";
		std::vector<char> msgBuf(msg.begin(), msg.end());
		msgBuf.push_back('\0');
		if (!yesnobox(msgBuf.data(), "Download and run mod?"))  return false;
	}

	if (strncmp(text, "http://", 7) == 0) {
		const size_t len = strlen(text);

		bool refresh = (len > 0 && text[len - 1] == '!');
		if (refresh) text[len - 1] = '\0';

		char dlCmd[1024];
#ifdef COMPILER_WINDOWS
		snprintf(dlCmd, sizeof(dlCmd), "@dl.dll -x -P \"webdls\" \"%s\"", text);
#else
		snprintf(dlCmd, sizeof(dlCmd), "wget -x -P \"webdls\" \"%s\"", text);
#endif

		memmove(text + 6, text + 7, strlen(text + 7) + 1);
		memcpy(text, "webdls", 6);

		char* q = strstr(text, "?");
		if (q) *q = '\0';

		std::ifstream inp(checkfilename(text));
		const bool cached = inp.good();
		inp.close();

#ifdef COMPILER_SYSTEM
		if (!cached || refresh) {
			ossystem(dlCmd, nullptr);
			mousebuttonbug(true);
		}
#endif

		replaceall(text, "%20", ' ');

#ifdef COMPILER_SYSTEM
		unzipInPlace(text);
#endif
		return true;
	}

	if (strlen(text) >= 2 && text[1] == ':') {
		if (doexit) exit(1);
		return false;
	}

	if (text[0] == '/' || text[0] == '\\') {
		if (doexit) exit(0);
		return false;
	}

#ifdef COMPILER_SYSTEM
	unzipInPlace(text);
#endif
	return true;
}

Token::Token(char* text) {
	this->text = text;
	length = strlen(text);
	pos = 0;
}

char* Token::getToken(bool math) {
	if (pos >= length) return nullptr;

	while (pos < length && (text[pos] == '\r' || text[pos] == '\n' || text[pos] == ' ' || text[pos] == '\t'))
		pos++;

	if (pos >= length) return nullptr;

	bool quote = false;
	int bracket = 0;
	bool bracket2 = false;
	long end = pos;
	atom = 0;

	if (text[pos] == '"') {
		quote = true;
		atom = TOKEN_ATOM_QUOTE;
	} else if (text[pos] == '(') {
		bracket = 1;
		atom = TOKEN_ATOM_BRACKET1;
	} else if (text[pos] == '<' && !math) {
		bracket2 = true;
		atom = TOKEN_ATOM_BRACKET2;
	}

	while (end < length && (quote || bracket || bracket2 || (text[end] != '\r' && text[end] != '\n' && text[end] != ' ' && text[end] != '\t'))) {
		end++;
		if (end < length) {
			if (bracket && text[end] == '(') bracket++;
			if (quote && text[end] == '"') quote = false;
			if (bracket && text[end] == ')') bracket--;
			if (bracket2 && text[end] == '>') bracket2 = false;
		}
	}

	char* token;
	if (atom != TOKEN_ATOM_QUOTE) {
		token = new char[end - pos + 1];
		memcpy(token, text + pos, end - pos);
		token[end - pos] = '\0';
	} else {
		token = new char[end - pos - 1];
		memcpy(token, text + pos + 1, end - pos - 2);
		token[end - pos - 2] = '\0';
	}

	pos = end;
	return token;
}

char* Token::getRest() {
	if (pos >= length - 1)
		return 0;
	int l = strlen(text + pos + 1);
	while (text[l + pos] == ' ')
		l--;
	if (l <= 0)
		return 0;
	char* token = new char[l + 1];
	memcpy(token, text + pos + 1, l);
	token[l] = 0;
	return token;
}

void Token::reset() {
	pos = 0;
}

char* Token::getuntillast() {
	int cur = 0, prev = 0, pprev = 0;

	while (true) {
		char* tmp = getToken(true);
		if (!tmp) break;
		delete[] tmp;
		pprev = prev;
		prev = cur;
		cur = pos;
	}

	pos = pprev;
	if (!pos || pos >= length)
		return nullptr;

	const char saved = text[pos];
	text[pos] = '\0';
	char* t = new char[strlen(text) + 1];
	strcpy(t, text);
	text[pos] = saved;
	return t;
}

int tmptriggers = 0;
Trigger* parseTrigger(char* name, int owner) {
	if (name) {
		if (!strcmp(name, "{")) {
			char* tmp = new char[32];
			snprintf(tmp, 32, "TMPTRIGGER%i", tmptriggers++);
			triggerstack.push(tmp);
			return findTrigger(triggerstack.top(), owner);
		}
		if (!strcmp(name, "THIS") && triggerstack.size())
			return findTrigger(triggerstack.top(), owner);
	}
	return findTrigger(name, owner);
}

int parseenc(char* data, int owner, char* filename) {
	Blowfish BF;
	char* pwd = new char[16];
	strcpy(pwd, "http://siebn.de");
	BF.Set_Passwd(pwd);
	int len;
	std::string s = base64_decode(data, &len);
	len = s.size();
	const char* st = s.c_str();
	char* bf = new char[len + 1];
	for (int i = 0; i < len; i++)
		bf[i] = st[i];
	BF.Decrypt(bf, s.size());
	secretcode++;
	parsechar(bf, owner, filename);
	secretcode--;
	delete bf;
	return 0;
}

int parsefile(char* filename, int owner) {
	const size_t flen = strlen(filename);

	auto endsWith = [&](const char* ext) {
		const size_t elen = strlen(ext);
		return (flen >= elen) && !strcmp(filename + flen - elen, ext);
		};

	if (endsWith(".png") || endsWith(".PNG") || endsWith(".bmp") || endsWith(".BMP")) {
		load(filename);
		return 0;
	}

	std::ifstream file(checkfilename(filename), std::ios::in | std::ios::ate | std::ios::binary);
	if (!file) {
		char tmp[10000];
		snprintf(tmp, sizeof(tmp), "Error opening file \"%s\"", filename);
		error(tmp, owner);
		return 0;
	}

	const long size = static_cast<long>(file.tellg());
	file.seekg(0, std::ios::beg);
	char* data = new char[size + 2];
	file.read(data, size);
	data[size] = 0;
	file.close();

	if (data[0] == '<') {
		if (strstr(data, "<bs2>") && strstr(data, "</bs2>")) {
			{
				std::ofstream savefile(checkfilename("xml2bs2.xml"), std::ios::out | std::ios::ate | std::ios::binary);
				savefile.write(data, strlen(data));
			}
			delete[] data;
			ossystem("Wscript", "xml2bs2.js");
			mousebuttonbug(true);
			return parsefile("xml2bs2.bs2", owner);
		} else {
			auto parseTagContent = [&](const char* openTag, const char* closeTag) {
				char* start = data;
				char* end = nullptr;
				while ((start = strstr(start, openTag)) != nullptr) {
					start = strstr(start, ">") + 1;
					if (!start) break;
					end = strstr(start, closeTag);
					if (!end) break;
					const char saved = *end;
					*end = 0;
					replacehtmlstrings(start);
					parsechar(start, owner, filename);
					*end = saved;
					start = end + 1;
				}
				};

			parseTagContent("<pre", "</pre>");
			parseTagContent("<code", "</code>");

			char* start = data;
			while ((start = strstr(start, "<td class=\"code\"")) != nullptr) {
				start = strstr(start, ">") + 1;
				if (!start) break;
				char* end = strstr(start, "</td>");
				if (!end) break;
				const char saved = *end;
				*end = 0;
				replacehtmlstrings(start);
				parsechar(start, owner, filename);
				*end = saved;
				start = end + 1;
			}
		}
	} else {
		if (endsWith(".bf") || endsWith(".BF")) {
			secretcode++;
			char* bf = bf_exec(data);
			parsechar(bf, owner, filename);
			delete[] bf;
			secretcode--;
		} else if (endsWith(".bs2e") || endsWith(".BS2E")) {
			parseenc(data, owner, filename);
		} else parsechar(data, owner, filename);
	}

	delete[] data;
	return 0;
}

int parsechar(char* text, int owner, char* filename) {
	if (!text || !text[0]) return 0;

	removeall(text, "\\\n", ' ');
	removeall(text, "\\ \n", ' ');
	removeall(text, "\\\r\n", ' ');
	removeall(text, "\\ \r\n", ' ');

	const long length = static_cast<long>(strlen(text));
	long pos = 0;
	long end = 0;
	int linenum = 0;

	while (pos < length) {
		while (end < length && text[end] != '\n') end++;

		char* line = new char[end - pos + 1];
		memcpy(line, text + pos, end - pos);
		line[end - pos] = '\0';
		pos = ++end;
		linenum++;

		parseline(line, linenum, owner, filename);
		delete[] line;
	}
	return 0;
}

int parseline(char* text, int linenum, int owner, char* filename) {
	static Var* debugparser = (Var*)setVar("DEBUGPARSER", 0);

	if (!filename) filename = "";

	if (debugparser->value) {
		const size_t len = strlen(text) + 512;
		char* tmp = new char[len];
		snprintf(tmp, len, "parsing line: messageid: %i, filename: %s, line: %i, line: %s", owner, filename, linenum, text);
		if (!secretcode) std::cout << tmp << std::endl;
		delete[] tmp;
	}

	const unsigned int i = static_cast<unsigned int>(triggerstack.size());
	char* triggername = i ? triggerstack.top() : nullptr;

	Token tokens(text);
	Action* action = parseaction(&tokens, owner);

	auto debugAction = [&](Action* a) {
		if (!debugparser->value) return;
		const size_t len = strlen(text) + 512;
		char* tmp = new char[len];
		char* str = a->toString();
		snprintf(tmp, len, "parsed action: messageid: %i, filename: %s, line: %i, action: %s", owner, filename, linenum, str);
		if (!secretcode) std::cout << tmp << std::endl;
		delete[] str;
		delete[] tmp;
		};

	if (!action) {
		if (i > triggerstack.size()) {
			if (triggerstack.empty()) {
				debugAction(actionstack.top());
				actionstack.top()->exec();
				delete actionstack.top();
				actionstack.pop();
			}
		} else if (drawobjectaction == reinterpret_cast<Action*>(-1)) {
			drawobjectaction = nullptr;
		} else if (!drawobjectaction) {
#if LOGLEVEL >= 3
			const size_t len = strlen(text) + 512;
			char* tmp = new char[len];
			if (!filename || !filename[0])
				snprintf(tmp, len, "ERROR in line %i: %s", linenum, text);
			else snprintf(tmp, len, "ERROR in file %s in line %i: %s", filename, linenum, text);
			if (!secretcode) error(tmp, owner);
			delete[] tmp;
#endif
		}
	} else {
		if (i) {
			addAction(triggername, action, owner);
		} else if (i < triggerstack.size()) {
			actionstack.push(action);
		} else {
			debugAction(action);
			action->exec();
			delete action;
		}
	}
	return 0;
}

Action* parseaction(Token* tokens, int owner) {
	static Var* autoexec = (Var*)setVar("AUTOEXEC", 0);
	static int buttonid = 1000;
	char* function = tokens->getToken();
	Action* action = NULL;
	if ((function == NULL) || (!strcmp(function, "//")) || (function[0] == '<')) {
		return new Action;
	}
	if ((function[0] == '+') || (function[0] == '-') || (function[0] == '<') || (function[0] == '>') || (function[0] == '.') || (function[0] == ',') || (function[0] == '[') || (function[0] == ']')) {
		char* out = bf_exec(tokens->text);
		secretcode++;
		parsechar(out, owner, "Brainfuck");
		secretcode--;
		delete out;
		action = new Action;
	}
	if (!strcmp(function, "}")) {
		if (drawobjectaction) {
			drawobjectaction = 0;
		}
		if (triggerstack.size()) {
			delete (triggerstack.top());
			triggerstack.pop();
		}
		if (ifaction) {
			char* t1 = tokens->getToken();
			char* t2 = tokens->getToken();
			if (t1 && t2 && !strcmp(t1, "ELSE")) {
				((ActionIf*)ifaction)->elsetrigger = parseTrigger(t2, owner);
				action = new Action();
			}
		}
		ifaction = 0;
	} else if (drawobjectaction) {
		char* data = new char[strlen(function) + 1];
		for (unsigned int i = 0; i < strlen(function); i++)
			if ((unsigned char)function[i] > 127) {
				char* tmp = new char[512];
				sprintf(tmp, "Found forbidden character \"%c\" in object. Ignoring line.", function[i]);
				error(tmp, owner);
				delete (tmp);
				delete (function);
				return 0;
			}
		strcpy(data, function);
		((ActionDrawObject*)drawobjectaction)->data.push_back(data);
	} else if ((function[0] == '!') && (strlen(function) > 2)) {
		parseenc(function + 1, owner, "Encrypted");
		action = new Action;
	} else if (!strcmp(function, "BS2Plus")) {
		action = new Action;
	} else if (!strcmp(function, "ERROR")) {
		action = new Action;
		char* t = tokens->getRest();
		if (t) {
			char* tmp = new char[strlen(t) + 512];
			sprintf(tmp, "ERROR from %i: %s", owner, t);
			error(tmp, 0);
			delete (tmp);
		}
	} else if (!strcmp(function, "TRIGGER")) {
		char* triggername = tokens->getToken();
		char* tmp = tokens->getToken();
		char* extend = 0;
		if (tmp && !strcmp(tmp, "EXTENDS")) {
			extend = tokens->getToken();
			delete (tmp);
			tmp = tokens->getToken();
		}
		if (triggername && tmp && !strcmp(tmp, "{")) {
			action = new ActionRemoveTrigger;
			((ActionRemoveTrigger*)action)->trigger = triggername;
			action->exec();
			if (extend) {
				Trigger* oldtrigger = findTrigger(extend, owner);

				std::list<Action*>::iterator it = oldtrigger->actions.begin();
				while (it != oldtrigger->actions.end()) {
					addAction(triggername, *it, owner);
					it++;
				}
			}
			action = new ActionTrigger;
			((ActionTrigger*)action)->triggername = 0;
			triggerstack.push(triggername);
		}
	} else if (!strcmp(function, "ON")) {
		char* triggername = tokens->getToken();
		if (triggername) {
			action = new ActionTrigger;
			((ActionTrigger*)action)->triggername = triggername;
			((ActionTrigger*)action)->action = parseaction(tokens, owner);
			if (!((ActionTrigger*)action)->action) {
				delete (action);
				action = NULL;
			}
		}
	} else if (!strcmp(function, "SAVE")) {
		char* screenname = tokens->getToken();
		char* filename = tokens->getToken();
		char* id = tokens->getToken();
		int screenid = -1;
		if (filename == 0) {
			filename = new char[1];
			*filename = 0;
		}
		if (screenname && checkfile(filename)) {
			if (!strcmp(screenname, "ALL"))
				screenid = ACTION_SAVE_SCREEN_ALL;
			else if (!strcmp(screenname, "SAND"))
				screenid = ACTION_SAVE_SCREEN_SAND;
			else if (!strcmp(screenname, "QUICKSAND"))
				screenid = ACTION_QUICKSAVE_SCREEN_SAND;
			else if (!strcmp(screenname, "STAMP"))
				screenid = ACTION_SAVE_STAMP;
			else if (!strcmp(screenname, "VAR"))
				screenid = ACTION_SAVE_VAR;
			else if (!strcmp(screenname, "TIMERS"))
				screenid = ACTION_SAVE_TIMERS;
			delete (screenname);
		}
		if ((screenid != -1) && (filename || (screenid == ACTION_SAVE_TIMERS))) {
			action = new ActionSave;
			if (screenid == ACTION_QUICKSAVE_SCREEN_SAND)
				((ActionSave*)action)->filename = (char*)(new Varint(filename));
			else
				((ActionSave*)action)->filename = filename;
			((ActionSave*)action)->screenid = screenid;
			((ActionSave*)action)->id = new Varint(id);
		}
	} else if (!strcmp(function, "LOAD")) {
		char* screenname = tokens->getToken();
		char* filename = tokens->getToken();
		char* id = tokens->getToken();
		int screenid = -1;
		if (screenname && checkfile(filename)) {
			if (!strcmp(screenname, "ALL"))
				screenid = ACTION_SAVE_SCREEN_ALL;
			else if (!strcmp(screenname, "SAND"))
				screenid = ACTION_SAVE_SCREEN_SAND;
			else if (!strcmp(screenname, "QUICKSAND"))
				screenid = ACTION_QUICKSAVE_SCREEN_SAND;
			else if (!strcmp(screenname, "STAMP"))
				screenid = ACTION_SAVE_STAMP;
			else if (!strcmp(screenname, "FGLAYER"))
				screenid = ACTION_SAVE_FGLAYER;
			else if (!strcmp(screenname, "BGLAYER"))
				screenid = ACTION_SAVE_BGLAYER;
			else if (!strcmp(screenname, "FONT"))
				screenid = ACTION_SAVE_FONT;
			else if (!strcmp(screenname, "MENUFONT"))
				screenid = ACTION_SAVE_MENUFONT;
			delete (screenname);
		}
		if ((screenid != -1) && filename) {
			action = new ActionLoad;
			if (screenid == ACTION_QUICKSAVE_SCREEN_SAND)
				((ActionLoad*)action)->filename = (char*)(new Varint(filename));
			else
				((ActionLoad*)action)->filename = filename;
			((ActionLoad*)action)->screenid = screenid;
			((ActionLoad*)action)->id = new Varint(id);
		}
	} else if (!strcmp(function, "RETURN")) {
		Varint* v = new Varint(tokens->getToken());
		if (v->ok && (tokens->getToken() == NULL)) {
			action = new ActionReturn();
			((ActionReturn*)action)->var = v;
		}
	} else if (!strcmp(function, "EXIT")) {
		if (tokens->getToken() == NULL) {
			action = new ActionExit();
		}
	} else if (!strcmp(function, "RESTART")) {
		char* t = tokens->getToken();
		if (tokens->getToken() == NULL) {
			action = new ActionRestart();
			((ActionRestart*)action)->parameter = t;
		}
	} else if (!strcmp(function, "TIMER")) {
		char* t = tokens->getToken();
		if (!strcmp(t, "CLEAR") && (tokens->getToken() == NULL)) {
			action = new ActionClearTimer;
			((ActionClearTimer*)action)->trigger = 0;
		}
		if (!strcmp(t, "REMOVE") || !strcmp(t, "REMOVEALL")) {
			char* trigger = tokens->getToken();
			if (trigger && (tokens->getToken() == NULL)) {
				action = new ActionClearTimer;
				((ActionClearTimer*)action)->trigger = parseTrigger(trigger, owner);
				((ActionClearTimer*)action)->removeall = !strcmp(t, "REMOVEALL");
			}
		} else {
			Varint* value = new Varint(t);
			char* what = tokens->getToken();
			char* trigger = tokens->getToken();
			Varint** param = getParameters(tokens);
			int type = -1;
			if (what) {
				if (!strcmp(what, "FRAMES"))
					type = ACTION_TIMER_FRAMES;
				else if (!strcmp(what, "SEK"))
					type = ACTION_TIMER_SEC;
				else if (!strcmp(what, "MSEK"))
					type = ACTION_TIMER_MSEC;
				delete (what);
			}
			Trigger* t = parseTrigger(trigger, owner);
			if (trigger && t && value->ok && (tokens->getToken() == NULL)) {
				action = new ActionTimer;
				((ActionTimer*)action)->value = value;
				((ActionTimer*)action)->type = type;
				((ActionTimer*)action)->trigger = t;
				((ActionTimer*)action)->params = param;
			}
		}
	} else if (!strcmp(function, "ELEMENT") || !strcmp(function, "Element")) {
		char* element_name = tokens->getToken();
		char* attribut = tokens->getToken();
		if (!strcmp(attribut, "DIE")) {
			char* dieto = tokens->getToken();
			Varint* rate = new Varint(tokens->getToken());
			if ((!tokens->getToken()) && dieto && rate->ok) {
				action = new ActionElementDie;
				((ActionElementDie*)action)->elementname = element_name;
				((ActionElementDie*)action)->dieto = dieto;
				((ActionElementDie*)action)->rate = rate;
			}
		} else {
			Varint* rvalue = new Varint(tokens->getToken(), 255);
			Varint* gvalue = new Varint(tokens->getToken(), 255);
			Varint* bvalue = new Varint(tokens->getToken(), 255);
			Varint* weight = new Varint(tokens->getToken(), 1000);
			if (!weight->ok)
				weight = new Varint(0);
			Varint* spay = new Varint(tokens->getToken(), 100);
			if (!spay->ok)
				spay = new Varint(1);
			Varint* slide = new Varint(tokens->getToken(), 100);
			if (!slide->ok)
				slide = new Varint(1);
			Varint* viscousity = new Varint(tokens->getToken(), 100);
			if (!viscousity->ok)
				viscousity = new Varint(1);
			Varint* dierate = new Varint(tokens->getToken(), 32768);
			if (!dierate->ok)
				dierate = new Varint(0);
			char* dieto = tokens->getToken();
			Varint* menuorder = new Varint(tokens->getToken());
			if (!menuorder->ok)
				menuorder = new Varint(0);
			char* tmp = tokens->getToken();
			char* tmp2 = tokens->getToken();
			Pic* icon = 0;
			if (tmp && tmp2) {
				checkfile(tmp2);
				icon = getPic(tmp, tmp2);
				if (!strcmp(icon->type, "HEX")) {
					icon->r = (void*)(new Varint(tokens->getToken(), 255));
					icon->g = (void*)(new Varint(tokens->getToken(), 255));
					icon->b = (void*)(new Varint(tokens->getToken(), 255));
				}
			} else {
				icon = getPic("TEXT", attribut);
			}
			if ((!tokens->getToken()) && rvalue->ok && gvalue->ok && bvalue->ok && weight->ok && spay->ok && slide->ok && viscousity->ok && dierate->ok && slide->ok && icon) {
				action = new ActionElementBS1;
				((ActionElementBS1*)action)->elementname = attribut;
				((ActionElementBS1*)action)->group = element_name;
				((ActionElementBS1*)action)->dieto = dieto;
				((ActionElementBS1*)action)->dierate = dierate;
				((ActionElementBS1*)action)->icon = icon;
				((ActionElementBS1*)action)->rvalue = rvalue;
				((ActionElementBS1*)action)->gvalue = gvalue;
				((ActionElementBS1*)action)->bvalue = bvalue;
				((ActionElementBS1*)action)->weight = weight;
				((ActionElementBS1*)action)->spay = spay;
				((ActionElementBS1*)action)->slide = slide;
				((ActionElementBS1*)action)->viscousity = viscousity;
				((ActionElementBS1*)action)->menuorder = menuorder;
			}
		}
	} else if (!strcmp(function, "INTERACTION") || !strcmp(function, "Interaction") || !strcmp(function, "INTERACTIONAT") || !strcmp(function, "InteractionAt")) {
		Varint* at;
		if (!strcmp(function, "INTERACTIONAT") || !strcmp(function, "InteractionAt"))
			at = new Varint(tokens->getToken());
		else
			at = new Varint(-1);
		char* element1 = tokens->getToken();
		char* element2 = tokens->getToken();
		if (element1 && element2) {
			action = new ActionInteraction;
			((ActionInteraction*)action)->elements1.push_back(element1);
			((ActionInteraction*)action)->elements2.push_back(element2);
			char* toself = tokens->getToken();
			char* toother = tokens->getToken();
			char* r = tokens->getToken();
			while (toself && toother && r) {
				((ActionInteraction*)action)->toselfs.push_back(toself);
				((ActionInteraction*)action)->toothers.push_back(toother);
				((ActionInteraction*)action)->rates.push_back(new Varint(r, 32768));
				toself = tokens->getToken();
				toother = tokens->getToken();
				r = tokens->getToken();
			}
			((ActionInteraction*)action)->except = toself;
			((ActionInteraction*)action)->at = at;
		}
	} else if (!strcmp(function, "INTERACTIONTRIGGER") || !strcmp(function, "InteractionTrigger") || !strcmp(function, "INTERACTIONTRIGGERAT") || !strcmp(function, "InteractionTriggerAt")) {
		Varint* at;
		if (!strcmp(function, "INTERACTIONTRIGGERAT") || !strcmp(function, "InteractionTriggerAt"))
			at = new Varint(tokens->getToken());
		else
			at = new Varint(-1);
		char* element1 = tokens->getToken();
		char* element2 = tokens->getToken();
		if (element1 && element2) {
			action = new ActionInteraction;
			((ActionInteraction*)action)->elements1.push_back(element1);
			((ActionInteraction*)action)->elements2.push_back(element2);
			char* trigger = tokens->getToken();
			char* r = tokens->getToken();
			while (trigger && r) {
				((ActionInteraction*)action)->triggers.push_back(parseTrigger(trigger, owner));
				((ActionInteraction*)action)->rates.push_back(new Varint(r, 32768));
				trigger = tokens->getToken();
				r = tokens->getToken();
			}
			((ActionInteraction*)action)->except = trigger;
			((ActionInteraction*)action)->at = at;
		}
	} else if (!strcmp(function, "INTERACTIONCLEAR")) {
		char* element = tokens->getToken();
		if (element && (tokens->getToken() == NULL)) {
			action = new ActionRemoveInteraction;
			((ActionRemoveInteraction*)action)->element = element;
			((ActionRemoveInteraction*)action)->index = NULL;
		}
	} else if (!strcmp(function, "INTERACTIONREMOVE")) {
		char* element = tokens->getToken();
		Varint* index = new Varint(tokens->getToken());
		if (element && (tokens->getToken() == NULL) && index->ok) {
			action = new ActionRemoveInteraction;
			((ActionRemoveInteraction*)action)->element = element;
			((ActionRemoveInteraction*)action)->index = index;
		}
	} else if (!strcmp(function, "DIECLEAR")) {
		char* element = tokens->getToken();
		if (tokens->getToken() == NULL) {
			action = new ActionClearDie;
			((ActionClearDie*)action)->element = element;
		}
	} else if (!strcmp(function, "ELEMENTSCLEAR")) {
		if (tokens->getToken() == NULL) {
			action = new ActionClearElements;
		}
	} else if (!strcmp(function, "WIND")) {
		Varint* x = new Varint(tokens->getToken());
		Varint* y = new Varint(tokens->getToken());
		Varint* d = new Varint(tokens->getToken());
		if (x->ok && y->ok && d->ok && tokens->getToken() == NULL) {
			action = new ActionWind;
			((ActionWind*)action)->x = x;
			((ActionWind*)action)->y = y;
			((ActionWind*)action)->d = d;
		}
	} else if (!strcmp(function, "NOBIAS")) {
		Varint* e = new Varint(tokens->getToken());
		if (e->ok && tokens->getToken() == NULL) {
			action = new ActionNoBias;
			((ActionNoBias*)action)->e = e;
		}
	} else if (!strcmp(function, "MENU")) {
		char* function2 = tokens->getToken();
		int where = -1;
		if (function2) {
			if (!strcmp(function2, "TOP"))
				where = MENU_BAR_TOP;
			else if (!strcmp(function2, "LEFT"))
				where = MENU_BAR_LEFT;
			else if (!strcmp(function2, "RIGHT"))
				where = MENU_BAR_RIGHT;
			else if (!strcmp(function2, "BOTTOM"))
				where = MENU_BAR_BOTTOM;
			else if (!strcmp(function2, "SUB"))
				where = MENU_BAR_SUB;
		}
		char* function3 = tokens->getToken();
		if (!strcmp(function2, "REFRESH") && !function3) {
			action = new ActionMenu;
			((ActionMenu*)action)->action = ACTION_MENU_REFRESH;
			((ActionMenu*)action)->bar = where;
		} else {
			if (!strcmp(function3, "CLEAR")) {
				action = new ActionMenu;
				((ActionMenu*)action)->action = ACTION_MENU_CLEAR;
				((ActionMenu*)action)->bar = where;
			}
			if (!strcmp(function3, "ADD") || !strcmp(function3, "ADDDRAG") || !strcmp(function3, "ADDBORDER")) {
				if (where != -1) {
					char* tiptypetmp = tokens->getToken();
					int tiptype = ACTION_BUTTON_TIPTYPE_TEXT;
					if (tiptypetmp) {
						if (!strcmp(tiptypetmp, "ELEMENT"))
							tiptype = ACTION_BUTTON_TIPTYPE_ELEMENT;
						if (!strcmp(tiptypetmp, "GROUP"))
							tiptype = ACTION_BUTTON_TIPTYPE_GROUP;
						delete (tiptypetmp);
					}
					char* tiptext = tokens->getToken();
					char* tmp = tokens->getToken();
					char* tmp2 = tokens->getToken();
					Pic* icon;
					if (tmp && tmp2) {
						if (!strcmp(tmp, "FILE"))
							checkfile(tmp2);
						icon = getPic(tmp, tmp2);
					} else {
						icon = getPic("", "");
					}
					Varint* r = 0;
					Varint* g = 0;
					Varint* b = 0;
					if (!strcmp(function3, "ADDBORDER")) {
						r = new Varint(tokens->getToken(), 255);
						g = new Varint(tokens->getToken(), 255);
						b = new Varint(tokens->getToken(), 255);
					}
					char* trigger_click = tokens->getToken();
					Varint** param = getParameters(tokens);
					if (tiptext && trigger_click && !tokens->getToken()) {
						Button* button;
						button = new Button;
						button->icon = icon;
						button->click = parseTrigger(trigger_click, owner);
						button->mode = !strcmp(function3, "ADDDRAG");
						button->id = buttonid++;
						button->tiptext = tiptext;
						action = new ActionButton;
						((ActionButton*)action)->button = button;
						((ActionButton*)action)->bar = where;
						((ActionButton*)action)->params = param;
						((ActionButton*)action)->tiptype = tiptype;
						((ActionButton*)action)->r = r;
						((ActionButton*)action)->g = g;
						((ActionButton*)action)->b = b;
					}
					if (action == 0) {
						delete (tiptext);
						delete (trigger_click);
						delete (param);
						delete (icon);
						delete (r);
						delete (g);
						delete (b);
					}
				}
			}
		}
		delete (function3);
	} else if (!strcmp(function, "SUBMENU")) {
		char* function = tokens->getToken();
		if (function && !strcmp(function, "CLOSE")) {
			action = new ActionSubMenu;
			((ActionSubMenu*)action)->function = 1;
		} else {
			Varint* x = 0, * y = 0;
			int align = 0;
			if (function && !strcmp(function, "HORIZONTAL")) {
				align = MENU_ALIGN_H;
				delete function;
				function = tokens->getToken();
			}
			if (function && !strcmp(function, "VERTICAL")) {
				align = MENU_ALIGN_V;
				delete function;
				function = tokens->getToken();
			}
			if (function && strcmp(function, "STAY")) {
				x = new Varint(function);
				y = new Varint(tokens->getToken());
				function = tokens->getToken();
			} else {
				x = new Varint((char*)0);
				y = new Varint((char*)0);
			}
			if (!tokens->getToken()) {
				action = new ActionSubMenu;
				((ActionSubMenu*)action)->function = 0;
				((ActionSubMenu*)action)->x = x;
				((ActionSubMenu*)action)->y = y;
				((ActionSubMenu*)action)->align = align;
				if (function && !strcmp(function, "STAY")) {
					((ActionSubMenu*)action)->stay = 1;
					delete function;
				} else
					((ActionSubMenu*)action)->stay = 0;
			}
		}
	} else if (!strcmp(function, "GROUP") || !strcmp(function, "Group")) {
		char* groupname = tokens->getToken();
		char* function2 = tokens->getToken();
		char* function3 = tokens->getToken();
		char* function4 = tokens->getToken();
		if (groupname && !function2) {
			if (!strcmp(groupname, "CLEARALL")) {
				action = new ActionGroup;
				((ActionGroup*)action)->function = ACTION_GROUP_CLEARALL;
				((ActionGroup*)action)->order = 0;
				((ActionGroup*)action)->groupname = 0;
				((ActionGroup*)action)->element = 0;
			}
		} else if (groupname && function2 && !function3) {
			if (!strcmp(function2, "CLEAR")) {
				action = new ActionGroup;
				((ActionGroup*)action)->groupname = groupname;
				((ActionGroup*)action)->function = ACTION_GROUP_CLEAR;
				((ActionGroup*)action)->order = 0;
				((ActionGroup*)action)->element = 0;
				delete (function2);
			}
		} else if (groupname && function2 && function3 && function4) {
			if (!strcmp(function2, "ADD")) {
				action = new ActionGroup;
				((ActionGroup*)action)->groupname = groupname;
				((ActionGroup*)action)->function = ACTION_GROUP_ADD;
				((ActionGroup*)action)->element = function3;
				((ActionGroup*)action)->order = new Varint(function4);
				delete (function2);
			} else {
				action = new ActionGroup;
				((ActionGroup*)action)->groupname = groupname;
				((ActionGroup*)action)->order = new Varint(function2);
				((ActionGroup*)action)->function = ACTION_GROUP_SET_ICON;
				((ActionGroup*)action)->element = 0;
				checkfile(function4);
				((ActionGroup*)action)->icon = getPic(function3, function4);
				if (!strcmp(function3, "HEX")) {
					((ActionGroup*)action)->icon->r = (void*)(new Varint(tokens->getToken(), 255));
					((ActionGroup*)action)->icon->g = (void*)(new Varint(tokens->getToken(), 255));
					((ActionGroup*)action)->icon->b = (void*)(new Varint(tokens->getToken(), 255));
				}
			}
		}
	} else if (!strcmp(function, "LIST")) {
		char* function2 = tokens->getToken();
		if (function2) {
			if (!strcmp(function2, "ELEMENTS") && !tokens->getToken()) {
				action = new ActionList;
				((ActionList*)action)->function = ACTION_LIST_ELEMENTS;
			}
			if (!strcmp(function2, "VARS") && !tokens->getToken()) {
				action = new ActionList;
				((ActionList*)action)->function = ACTION_LIST_VARS;
			}
			if (!strcmp(function2, "GROUPS") && !tokens->getToken()) {
				action = new ActionList;
				((ActionList*)action)->function = ACTION_LIST_GROUPS;
			}
			if (!strcmp(function2, "TRIGGERS") && !tokens->getToken()) {
				action = new ActionList;
				((ActionList*)action)->function = ACTION_LIST_TRIGGERS;
			}
			if (!strcmp(function2, "TRIGGEREXECS") && !tokens->getToken()) {
				action = new ActionList;
				((ActionList*)action)->function = ACTION_LIST_TRIGGEREXECS;
			}
			if (!strcmp(function2, "TIMERS") && !tokens->getToken()) {
				action = new ActionList;
				((ActionList*)action)->function = ACTION_LIST_TIMERS;
			}
			if (!strcmp(function2, "ACTIONS") || !strcmp(function2, "GROUP") || !strcmp(function2, "INTERACTIONS") || !strcmp(function2, "DIETOS") || !strcmp(function2, "ELEMENTGROUPS")) {
				char* element = tokens->getToken();
				if (element && !tokens->getToken()) {
					action = new ActionList;
					if (!strcmp(function2, "GROUP"))
						((ActionList*)action)->function = ACTION_LIST_GROUP;
					if (!strcmp(function2, "INTERACTIONS"))
						((ActionList*)action)->function = ACTION_LIST_INTERACTIONS;
					if (!strcmp(function2, "DIETOS"))
						((ActionList*)action)->function = ACTION_LIST_DIETOS;
					if (!strcmp(function2, "ELEMENTGROUPS"))
						((ActionList*)action)->function = ACTION_LIST_ELEMENTGROUPS;
					if (!strcmp(function2, "ACTIONS"))
						((ActionList*)action)->function = ACTION_LIST_ACTIONS;
					((ActionList*)action)->element = element;
				}
			}
		}
		delete (function2);
	} else if (!strcmp(function, "SET") || !strcmp(function, "Set")) {
		char* varname = tokens->getToken();
		Varint* v = new Varint(tokens->getToken());
		if (varname && v->ok && (tokens->getToken() == NULL)) {
			action = new ActionSetVar;
			((ActionSetVar*)action)->var = varname;
			((ActionSetVar*)action)->value = v;
			((ActionSetVar*)action)->t = 0;
		}
	} else if (!strcmp(function, "INC")) {
		char* varname = tokens->getToken();
		if (varname && (tokens->getToken() == NULL)) {
			action = new ActionInc;
			((ActionInc*)action)->var = varname;
			char* t = new char[strlen(varname) + 1];
			strcpy(t, varname);
			((ActionSetVar*)action)->value = new Varint(t);
			((ActionInc*)action)->t = 0;
		}
	} else if (!strcmp(function, "COUNT")) {
		char* varname = tokens->getToken();
		Varint* e = new Varint(tokens->getToken());
		if (varname && e->ok) {
			action = new ActionCount;
			((ActionCount*)action)->var = varname;
			((ActionCount*)action)->element = e;
			char* t1 = tokens->getToken();
			char* t2 = tokens->getToken();
			Varint* x = new Varint(tokens->getToken());
			Varint* y = new Varint(tokens->getToken());
			Varint* w = new Varint(tokens->getToken());
			Varint* h = new Varint(tokens->getToken());
			if (t1 && t2 && !strcmp(t1, "IN") && !strcmp(t2, "RECT") && x->ok && y->ok && w->ok && h->ok) {
				((ActionCount*)action)->x = x;
				((ActionCount*)action)->y = y;
				((ActionCount*)action)->w = w;
				((ActionCount*)action)->h = h;
			} else {
				((ActionCount*)action)->x = 0;
				((ActionCount*)action)->y = 0;
				((ActionCount*)action)->w = 0;
				((ActionCount*)action)->h = 0;
			}
			delete (t1);
			delete (t2);
		}
	} else if (!strcmp(function, "CLOSEST")) {
		Varint* e = new Varint(tokens->getToken());
		Varint* x = new Varint(tokens->getToken());
		Varint* y = new Varint(tokens->getToken());
		Varint* d = new Varint(tokens->getToken());
		if (x->ok && y->ok && e->ok && (tokens->getToken() == NULL)) {
			action = new ActionClosest;
			((ActionClosest*)action)->e = e;
			((ActionClosest*)action)->x = x;
			((ActionClosest*)action)->y = y;
			((ActionClosest*)action)->d = d;
		}
	} else if (!strcmp(function, "GET")) {
		char* v = tokens->getToken();
		if (v && (tokens->getToken() == NULL)) {
			action = new ActionGetVar;
			((ActionGetVar*)action)->value = v;
		}
	} else if (!strcmp(function, "GETFILE")) {
		char* v = tokens->getToken();
		if (v && (tokens->getToken() == NULL)) {
			action = new ActionGetFile;
			((ActionGetFile*)action)->filename = v;
		}
	} else if (!strcmp(function, "RESIZE")) {
		Varint* w = new Varint(tokens->getToken(), 420);
		Varint* h = new Varint(tokens->getToken(), 420);
		if (h->ok && w->ok && (tokens->getToken() == NULL)) {
			action = new ActionResize;
			((ActionResize*)action)->h = h;
			((ActionResize*)action)->w = w;
		}
	} else if (!strcmp(function, "SCROLL")) {
		Varint* x = new Varint(tokens->getToken(), 420);
		Varint* y = new Varint(tokens->getToken(), 420);
		if (x->ok && y->ok && (tokens->getToken() == NULL)) {
			action = new ActionScroll;
			((ActionScroll*)action)->x = x;
			((ActionScroll*)action)->y = y;
		}
	} else if (strstr(function, "http://") == function) {
		char* filename = new char[strlen(function) + 1];
		strcpy(filename, function);
		action = new ActionInclude;
		((ActionInclude*)action)->filename = filename;
	} else if (!strcmp(function, "INCLUDE") || !strcmp(function, "Include")) {
		char* filename = tokens->getToken();
		char* par = tokens->getToken();
		if (filename) {
			action = new ActionInclude;
			((ActionInclude*)action)->filename = filename;
			((ActionInclude*)action)->param = par;
		}
	} else if (!strcmp(function, "KEYCODE")) {
		Varint* v = new Varint(tokens->getToken());
		char* keyname = tokens->getToken();
		if (keyname && v->ok & (tokens->getToken() == NULL)) {
			action = new ActionKey;
			((ActionKey*)action)->v = v;
			((ActionKey*)action)->keyname = keyname;
		}
	} else if (!strcmp(function, "WHILE")) {
		Varint* v = new Varint(tokens->getToken());
		char* trigger = tokens->getToken();
		Varint** param = getParameters(tokens);
		if (trigger && v->ok && (tokens->getToken() == NULL)) {
			action = new ActionWhile;
			((ActionWhile*)action)->value = v;
			((ActionWhile*)action)->trigger = parseTrigger(trigger, owner);
			((ActionWhile*)action)->params = param;
		}
	} else if (!strcmp(function, "IF")) {
		Varint* v = new Varint(tokens->getToken());
		char* trigger = tokens->getToken();
		Varint** param = getParameters(tokens, "ELSE");
		char* trigger2 = 0;
		if (param == (Varint**)-1) {
			param = 0;
			trigger2 = tokens->getToken();
		}
		unsigned int tsize = triggerstack.size();
		Trigger* t = parseTrigger(trigger, owner);
		if (trigger && v->ok && t && (tokens->getToken() == NULL)) {
			action = new ActionIf;
			if (triggerstack.size() != tsize)
				ifaction = action;
			((ActionIf*)action)->value = v;
			((ActionIf*)action)->trigger = t;
			((ActionIf*)action)->elsetrigger = findTrigger(trigger2, owner);
			((ActionIf*)action)->params = param;
		}
	} else if (!strcmp(function, "FOR")) {
		char* v = tokens->getToken();
		if (!strcmp(v, "EACH")) {
			Varint* element = new Varint(tokens->getToken());
			char* dochar = tokens->getToken();
			char* trigger = tokens->getToken();
			Varint** param = getParameters(tokens);
			Trigger* t = parseTrigger(trigger, owner);
			if (trigger && element->ok && !strcmp(dochar, "DO")) {
				action = new ActionForEach;
				((ActionForEach*)action)->element = element;
				((ActionForEach*)action)->trigger = t;
				((ActionForEach*)action)->params = param;
			}
		} else {
			char* ischar = tokens->getToken();
			Varint* fromvar = new Varint(tokens->getToken());
			char* tochar = tokens->getToken();
			Varint* tovar = new Varint(tokens->getToken());
			char* dochar = tokens->getToken();
			Varint* step;
			if (!strcmp(dochar, "STEP")) {
				step = new Varint(tokens->getToken());
				delete (dochar);
				dochar = tokens->getToken();
			} else step = new Varint(1);
			char* trigger = tokens->getToken();
			Varint** param = getParameters(tokens);
			Trigger* t = parseTrigger(trigger, owner);
			if (trigger && !strcmp(ischar, "FROM") && (!strcmp(tochar, "TO") || !strcmp(tochar, "DOWNTO")) && !strcmp(dochar, "DO") && t && (tokens->getToken() == NULL)) {
				action = new ActionFor;
				((ActionFor*)action)->value = v;
				((ActionFor*)action)->fromvalue = fromvar;
				((ActionFor*)action)->tovalue = tovar;
				((ActionFor*)action)->trigger = t;
				((ActionFor*)action)->params = param;
				((ActionFor*)action)->step = step;
				if (!strcmp(tochar, "DOWNTO"))
					((ActionFor*)action)->function = FOR_DOWNTO;
				if (!strcmp(tochar, "TO"))
					((ActionFor*)action)->function = FOR_TO;
				delete (ischar);
				delete (tochar);
				delete (dochar);
			}
		}
	} else if (!strcmp(function, "EXEC")) {
		char* trigger = tokens->getToken();
		Varint** param = getParameters(tokens);
		if (trigger && (tokens->getToken() == NULL)) {
			action = new ActionExec;
			((ActionExec*)action)->trigger = parseTrigger(trigger, owner);
			((ActionExec*)action)->params = param;
		}
	} else if (!strcmp(function, "SYSTEM")) {
		char* cmd = tokens->getToken();
		if (cmd && (tokens->getToken() == NULL)) {
			char* replace;
			while (replace = strchr(cmd, ';'))
				*replace = '\n';
			action = new ActionSystem;
			((ActionSystem*)action)->cmd = cmd;
		}
	} else if (!strcmp(function, "REMOVETRIGGER")) {
		char* trigger = tokens->getToken();
		if (trigger && (tokens->getToken() == NULL)) {
			action = new ActionRemoveTrigger;
			((ActionRemoveTrigger*)action)->trigger = trigger;
		}
	} else if (!strcmp(function, "Message")) {
		action = new ActionMessage;
		((ActionMessage*)action)->function = MESSAGE_BS1;
		((ActionMessage*)action)->varint = new Varint((char*)0);
		((ActionMessage*)action)->message = tokens->getRest();
	} else if (!strcmp(function, "MESSAGE")) {
		char* function2 = tokens->getToken();
		char* message = tokens->getToken();
		Varint* tmp = 0;
		int f = -1;
		if (function2) {
			if (!strcmp(function2, "CLEAR")) {
				f = MESSAGE_CLEAR;
				message = "";
			} else if (!strcmp(function2, "ADDTEXT"))
				f = MESSAGE_ADDTEXT;
			else if (!strcmp(function2, "ADDNUMBER"))
				f = MESSAGE_ADDNUMBER;
			else if (!strcmp(function2, "ADDELEMENT"))
				f = MESSAGE_ADDELEMENT;
			else if (!strcmp(function2, "ADDELEMENTTEXT"))
				f = MESSAGE_ADDELEMENTTEXT;
			else if (!strcmp(function2, "ADDGROUP"))
				f = MESSAGE_ADDGROUP;
			else if (!strcmp(function2, "SEND")) {
				f = MESSAGE_SEND;
				if (!message) message = "";
			} else if (!strcmp(function2, "SAVE")) {
				f = MESSAGE_SAVE;
				message = "";
			} else if (!strcmp(function2, "MESSAGEBOX")) {
				f = MESSAGE_MESSAGEBOX;
				message = "";
			} else if (!strcmp(function2, "EXEC")) {
				f = MESSAGE_EXEC;
				if (!message) message = "";
			} else if (!strcmp(function2, "SENDTEXT")) {
				f = MESSAGE_SENDTEXT;
				tmp = new Varint(tokens->getToken());
			} else if (!strcmp(function2, "ADDLINE")) {
				f = MESSAGE_ADDLINE;
				message = "";
			} else if (!strcmp(function2, "SYSTEM")) {
				f = MESSAGE_SYSTEM;
				message = "";
			} else if (!strcmp(function2, "SAVESTRING")) {
				f = MESSAGE_SAVESTRING;
			} else if (!strcmp(function2, "ADDSTRING")) {
				f = MESSAGE_ADDSTRING;
			}
			delete (function2);
		}
		if ((f != -1) && message && (tokens->getToken() == NULL)) {
			action = new ActionMessage;
			((ActionMessage*)action)->function = f;
			((ActionMessage*)action)->message = message;
			if ((f == MESSAGE_SAVESTRING) || (f == MESSAGE_ADDSTRING) || (f == MESSAGE_ADDNUMBER) || (f == MESSAGE_ADDELEMENT) || (f == MESSAGE_ADDELEMENTTEXT) || (f == MESSAGE_ADDGROUP) || (f == MESSAGE_SEND) || (f == MESSAGE_EXEC))
				((ActionMessage*)action)->varint = new Varint(message);
			else if (f == MESSAGE_SENDTEXT)
				((ActionMessage*)action)->varint = tmp;
			else
				((ActionMessage*)action)->varint = 0;
		}
	} else if (!strcmp(function, "FILE")) {
		char* function2 = tokens->getToken();
		char* filename = tokens->getToken();
		char* par = tokens->getToken();
		if (!strcmp(function2, "OPEN") && checkfile(filename) && (tokens->getToken() == NULL)) {
			action = new ActionFile;
			((ActionFile*)action)->filename = filename;
			((ActionFile*)action)->function = ACTION_FILE_OPEN;
			((ActionFile*)action)->param = par;
		}
		if (!strcmp(function2, "CLOSE") && (filename == NULL)) {
			action = new ActionFile;
			((ActionFile*)action)->filename = filename;
			((ActionFile*)action)->function = ACTION_FILE_CLOSE;
		}
		if (!strcmp(function2, "DELETE") && checkfile(filename) && (tokens->getToken() == NULL)) {
			action = new ActionFile;
			((ActionFile*)action)->filename = filename;
			((ActionFile*)action)->function = ACTION_FILE_DELETE;
		}
	} else if (!strcmp(function, "STATUS")) {
		char* function = tokens->getToken();
		char* text = tokens->getToken();
		int f = -1;
		if (function) {
			if (!strcmp(function, "CLEAR")) {
				f = STATUS_CLEAR;
				text = "ALL";
			} else if (!strcmp(function, "ADDTEXT"))
				f = STATUS_ADDTEXT;
			else if (!strcmp(function, "ADDNUMBER"))
				f = STATUS_ADDNUMBER;
			else if (!strcmp(function, "ADDELEMENT"))
				f = STATUS_ADDELEMENT;
			else if (!strcmp(function, "ADDGROUP"))
				f = STATUS_ADDGROUP;
			else if (!strcmp(function, "ADDMOUSEOVER")) {
				f = STATUS_MOUSEOVER;
				text = "ALL";
			}
			delete (function);
		}
		if ((f != -1) && text && (tokens->getToken() == NULL)) {
			action = new ActionStatus;
			((ActionStatus*)action)->function = f;
			((ActionStatus*)action)->text = text;
			((ActionStatus*)action)->v = 0;
			if ((f == STATUS_ADDNUMBER) || (f == STATUS_ADDELEMENT) || (f == STATUS_ADDGROUP))
				((ActionStatus*)action)->v = new Varint(text);
		}
	} else if (!strcmp(function, "CONNECT")) {
		char* host = tokens->getToken();
		Varint* port = new Varint(tokens->getToken());
		char* var = tokens->getToken();
		if (host && port->ok && var && !tokens->getToken()) {
			action = new ActionConnect;
			((ActionConnect*)action)->host = host;
			((ActionConnect*)action)->port = port;
			((ActionConnect*)action)->var = var;
		}
	} else if (!strcmp(function, "REMOTE")) {
		Varint* mid = new Varint(tokens->getToken());
		char* type = tokens->getToken();
		if (mid->ok && type) {
			if (!strcmp(type, "SET")) {
				char* text = tokens->getToken();
				Varint* val = new Varint(tokens->getToken());
				if (text && val->ok && !tokens->getToken()) {
					action = new ActionRemote;
					((ActionRemote*)action)->type = REMOTE_SET;
					((ActionRemote*)action)->text = text;
					((ActionRemote*)action)->val = val;
					((ActionRemote*)action)->mid = mid;
				}
			} else if (!strcmp(type, "EXEC")) {
				char* text = tokens->getToken();
				if (text && !tokens->getToken()) {
					action = new ActionRemote;
					((ActionRemote*)action)->type = REMOTE_EXEC;
					((ActionRemote*)action)->text = text;
					((ActionRemote*)action)->val = 0;
					((ActionRemote*)action)->mid = mid;
				}
			}
			delete (type);
		}
	} else if (!strcmp(function, "WRITE")) {
		Varint* element = new Varint(tokens->getToken());
		Varint* x = new Varint(tokens->getToken());
		Varint* y = new Varint(tokens->getToken());
		Varint* size = new Varint(tokens->getToken());
		char* type = tokens->getToken();
		bool align = 0;
		int f = 0;
		if (type && !strcmp(type, "CENTER")) {
			delete (type);
			align = 1;
			type = tokens->getToken();
		}
		char* text = tokens->getToken();
		if (type) {
			if (!strcmp(type, "TEXT"))
				f = WRITE_TEXT;
			else if (!strcmp(type, "NUMBER"))
				f = WRITE_NUMBER;
			else if (!strcmp(type, "ELEMENT"))
				f = WRITE_ELEMENT;
			else if (!strcmp(type, "GROUP"))
				f = WRITE_GROUP;
			else if (!strcmp(type, "STRING"))
				f = WRITE_STRING;
			else if (!strcmp(type, "MESSAGE")) {
				f = WRITE_MESSAGE;
				text = new char[1];
				text[0] = 0;
			}
			delete (type);
		}
		if (text && (x->ok) && (y->ok) && (size->ok) && f && (tokens->getToken() == NULL)) {
			action = new ActionWrite;
			((ActionWrite*)action)->element = element;
			((ActionWrite*)action)->align = align;
			((ActionWrite*)action)->type = f;
			if (f == WRITE_TEXT)
				((ActionWrite*)action)->v = 0;
			else {
				((ActionWrite*)action)->v = new Varint(text);
			}
			((ActionWrite*)action)->x = x;
			((ActionWrite*)action)->y = y;
			((ActionWrite*)action)->size = size;
			if (f == WRITE_TEXT)
				((ActionWrite*)action)->text = text;
			else
				((ActionWrite*)action)->text = 0;
		}
	} else if (!strcmp(function, "DRAW")) {
		int b = -1;
		Varint* element = new Varint(tokens->getToken());
		char* brush = tokens->getToken();
		if (brush) {
			if (!strcmp(brush, "FILLEDCIRCLE"))
				b = BRUSH_FILLEDCIRCLE;
			else if (!strcmp(brush, "CIRCLE"))
				b = BRUSH_CIRCLE;
			else if (!strcmp(brush, "RECT"))
				b = BRUSH_RECT;
			else if (!strcmp(brush, "FILLEDRECT"))
				b = BRUSH_FILLEDRECT;
			else if (!strcmp(brush, "LINE"))
				b = BRUSH_LINE;
			else if (!strcmp(brush, "FILL"))
				b = BRUSH_FILL;
			else if (!strcmp(brush, "REPLACEFILLEDCIRCLE"))
				b = REPLACE_FILLEDCIRCLE;
			else if (!strcmp(brush, "REPLACELINE"))
				b = REPLACE_LINE;
			else if (!strcmp(brush, "COPYRECT"))
				b = COPY_RECT;
			else if (!strcmp(brush, "ROTATE"))
				b = ROTATE_RECT;
			else if (!strcmp(brush, "POINT"))
				b = BRUSH_POINT;
			else if (!strcmp(brush, "RANDOMFILLEDCIRCLE"))
				b = BRUSH_RADNOMFILLEDCIRCLE;
			else if (!strcmp(brush, "COPYSTAMP"))
				b = COPY_STAMP;
			else if (!strcmp(brush, "PASTESTAMP"))
				b = PASTE_STAMP;
			else if (!strcmp(brush, "SWAPPOINTS"))
				b = BRUSH_SWAPPOINTS;
			else if (!strcmp(brush, "FILLEDELLIPSE"))
				b = BRUSH_FILLEDELLIPSE;
			else if (!strcmp(brush, "ELLIPSE"))
				b = BRUSH_ELLIPSE;
			else if (!strcmp(brush, "REPLACEFILLEDELLIPSE"))
				b = REPLACE_FILLEDELLIPSE;
			else if (!strcmp(brush, "RANDOMFILLEDELLIPSE"))
				b = BRUSH_RANDOMFILLEDELLIPSE;
			else if (!strcmp(brush, "POINTS"))
				b = BRUSH_POINTS;
			else if (!strcmp(brush, "OBJECT"))
				b = BRUSH_OBJECT;
			delete (brush);
		}
		if (b == BRUSH_OBJECT) {
			action = new ActionDrawObject;
			for (int i = 0; i < 256; i++)
				((ActionDrawObject*)action)->elements[i] = 0;
			((ActionDrawObject*)action)->xoffset = new Varint(tokens->getToken());
			((ActionDrawObject*)action)->yoffset = new Varint(tokens->getToken());
			((ActionDrawObject*)action)->sizex = 0;
			((ActionDrawObject*)action)->sizey = 0;
			char* c = tokens->getToken();
			char* e = tokens->getToken();
			while (c && e) {
				if (!strcmp(c, "SIZEX")) {
					((ActionDrawObject*)action)->sizex = new Varint(e);
					c = tokens->getToken();
					e = tokens->getToken();
				} else if (!strcmp(c, "SIZEY")) {
					((ActionDrawObject*)action)->sizey = new Varint(e);
					c = tokens->getToken();
					e = tokens->getToken();
				} else if (!strcmp(c, "SIZE")) {
					((ActionDrawObject*)action)->sizex = new Varint(e);
					((ActionDrawObject*)action)->sizey = ((ActionDrawObject*)action)->sizex;
					c = tokens->getToken();
					e = tokens->getToken();
				} else {
					((ActionDrawObject*)action)->elements[(int)c[0]] = new Varint(e);
					delete (c);
					c = tokens->getToken();
					e = tokens->getToken();
				}
			}
			if (c && !strcmp(c, "{")) {
				parseTrigger(c, owner);
				drawobjectaction = action;
			} else {
				delete (action);
				action = 0;
			}
		} else if (b == BRUSH_POINTS) {
			action = new ActionDrawPoints;
			((ActionDrawPoints*)action)->xoffset = new Varint(tokens->getToken());
			((ActionDrawPoints*)action)->yoffset = new Varint(tokens->getToken());
			int ix, iy;
			char* x = tokens->getToken();
			char* y = tokens->getToken();
			while (x && y) {
				if (sscanf(x, "%i", &ix) && sscanf(y, "%i", &iy)) {
					((ActionDrawPoints*)action)->element = element;
					((ActionDrawPoints*)action)->x.push_back(ix);
					((ActionDrawPoints*)action)->y.push_back(iy);
				}
				x = tokens->getToken();
				y = tokens->getToken();
			}
		} else {
			Varint* x = new Varint(tokens->getToken());
			Varint* y = new Varint(tokens->getToken());
			Varint* dx = new Varint(tokens->getToken());
			Varint* dy = new Varint(tokens->getToken());
			Varint* a1 = new Varint(tokens->getToken());
			Varint* a2 = new Varint(tokens->getToken());
			if ((b != -1) && (element->ok) && (x->ok) && (y->ok) && (tokens->getToken() == NULL)) {
				action = new ActionDraw;
				((ActionDraw*)action)->brush = b;
				((ActionDraw*)action)->element = element;
				((ActionDraw*)action)->drawx = x;
				((ActionDraw*)action)->drawy = y;
				((ActionDraw*)action)->dx = dx;
				((ActionDraw*)action)->dy = dy;
				((ActionDraw*)action)->a1 = a1;
				((ActionDraw*)action)->a2 = a2;
			}
		}
	} else {
		char* equals = tokens->getToken();
		if (equals && !strcmp(equals, "=")) {
			Varint* v = new Varint(tokens->getToken());
			if (v->ok && (tokens->getToken() == NULL)) {
				action = new ActionSetVar;
				((ActionSetVar*)action)->var = function;
				((ActionSetVar*)action)->value = v;
				((ActionSetVar*)action)->t = 0;
			}
		} else if (autoexec->value) {
			delete (function);
			delete (equals);
			tokens->reset();
			function = tokens->getToken();
			Varint** param = getParameters(tokens);
			if (tokens->getToken() == NULL) {
				action = new ActionExec;
				((ActionExec*)action)->trigger = parseTrigger(function, owner);
				((ActionExec*)action)->params = param;
			}
		} else {
			delete (function);
			delete (equals);
		}
	}
	if (action) action->owner = owner;
	return action;
}

Varint** getParameters(Token* tokens, char* exception) {
	Varint** r = new Varint * [11];
	for (int i = 0; i < 11; i++) r[i] = nullptr;

	for (int i = 0; i < 10; i++) {
		char* tmp = tokens->getToken();

		if ((i == 0) && tmp && exception && !strcmp(exception, tmp)) {
			delete[] r;
			delete[] tmp;
			return reinterpret_cast<Varint**>(-1);
		}

		Varint* t = new Varint(tmp);
		if (t->ok) {
			r[i] = t;
		} else {
			delete t;
			if (i == 0) {
				delete[] r;
				return nullptr;
			}
			return r;
		}
	}

	return r;
}