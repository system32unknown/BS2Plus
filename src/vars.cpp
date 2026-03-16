#include "vars.h"
#include "config.h"
#include "sdlbasics.h"
#include <cmath>
#include <cstring>
#include <cstdio>
#include <iostream>

// C++17: inline constexpr replaces #define PI
inline constexpr double PI = 3.141592654;

#define get_e int e = 0; if (!getVar(tmp, &e)) e = findElement(tmp, true);

std::list<Var*> vars;
int* parameters[MAXPARAMETERS + 1];
int parameterpos = -1;
Var* debugparameter;
Var* debugvar;

std::list<Var*>* getVars() {
	return &vars;
}

uintptr_t setVar(char* name, int value, bool set) {
	char tmp[255];
	if (sscanf(name, "ELEMENT%254s", tmp)) {
		if (set) {
			if (sscanf(tmp, "WEIGHT:%254s", tmp)) {
				get_e
					setElementWeight(e, value);
				return 0;
			} else if (sscanf(tmp, "SPRAY:%254s", tmp)) {
				get_e
					setElementSpray(e, value);
				return 0;
			} else if (sscanf(tmp, "SLIDE:%254s", tmp)) {
				get_e
					setElementSlide(e, value);
				return 0;
			} else if (sscanf(tmp, "VISCOUSITY:%254s", tmp)) {
				get_e
					setElementViscousity(e, value);
				return 0;
			} else if (sscanf(tmp, "R:%254s", tmp)) {
				get_e
					setElementR(e, value);
				return 0;
			} else if (sscanf(tmp, "G:%254s", tmp)) {
				get_e
					setElementG(e, value);
				return 0;
			} else if (sscanf(tmp, "B:%254s", tmp)) {
				get_e
					setElementB(e, value);
				return 0;
			} else if (sscanf(tmp, "CUSTOMR1:%254s", tmp)) {
				get_e
					setElementCustomColor(e, 1, 1, value);
				return 0;
			} else if (sscanf(tmp, "CUSTOMG1:%254s", tmp)) {
				get_e
					setElementCustomColor(e, 1, 2, value);
				return 0;
			} else if (sscanf(tmp, "CUSTOMB1:%254s", tmp)) {
				get_e
					setElementCustomColor(e, 1, 3, value);
				return 0;
			} else if (sscanf(tmp, "CUSTOMR2:%254s", tmp)) {
				get_e
					setElementCustomColor(e, 2, 1, value);
				return 0;
			} else if (sscanf(tmp, "CUSTOMG2:%254s", tmp)) {
				get_e
					setElementCustomColor(e, 2, 2, value);
				return 0;
			} else if (sscanf(tmp, "CUSTOMB2:%254s", tmp)) {
				get_e
					setElementCustomColor(e, 2, 3, value);
				return 0;
			} else if (sscanf(tmp, "CUSTOMR3:%254s", tmp)) {
				get_e
					setElementCustomColor(e, 3, 1, value);
				return 0;
			} else if (sscanf(tmp, "CUSTOMG3:%254s", tmp)) {
				get_e
					setElementCustomColor(e, 3, 2, value);
				return 0;
			} else if (sscanf(tmp, "CUSTOMB3:%254s", tmp)) {
				get_e
					setElementCustomColor(e, 3, 3, value);
				return 0;
			} else if (sscanf(tmp, "DEATHRATE:%254s", tmp)) {
				unsigned int i;
				if (sscanf(tmp, "%i:%254s", &i, tmp) == 2) {
					get_e
						if ((getElement(e)->dies)->empty())
							return 0;
					auto it2 = getElement(e)->dies->begin();
					if (i <= static_cast<unsigned int>((getElement(e)->dies)->size()))
						for (unsigned int ii = 0; ii < i; ii++) ++it2;
					(*it2)->rate = value;
					setprecalc(3);
					return 0;
				}
			}
		} else
			return 0;
	}

	if (strchr(name, '[') && (strchr(name, '[') < strchr(name, ']'))) {
		char* t2 = strchr(name, '[');
		char* t3 = strchr(name, ']');
		*t2 = 0;
		*t3 = 0;
		int var = 0;
		getVar(t2 + 1, &var);
		snprintf(tmp, sizeof(tmp), "%s%i", name, var);
		*t2 = '[';
		*t3 = ']';
		return setVar(tmp, value, set);
	}

	for (auto* v : vars) {
		if (!strcmp(v->name, name)) {
			if (set) v->value = value;
			return reinterpret_cast<uintptr_t>(v);
		}
	}

	Var* v = new Var;
	strcpy(v->name, name);
	v->value = value;
	vars.push_back(v);
	return (uintptr_t)v;
}

int getVar(char* name, int* value) {
	if (!name) { *value = 0; return false; }
	char tmp[255];
	if (name[0] == '$') {
		if ((parameterpos != -1) && (parameters[parameterpos] != nullptr) && (name[2] == '\0')) {
			int i = name[1] - '0';
			if ((i >= 0) && (i < 10)) {
				*value = parameters[parameterpos][i];
				return true;
			} else {
				*value = 0;
				return false;
			}
		} else {
			*value = 0;
			return false;
		}
	}

	if (sscanf(name, "ELEMENT%254s", tmp)) {
		if (!strcmp(tmp, ":COUNT")) {
			*value = getelementscount();
			return true;
		} else if (sscanf(tmp, ":%254s", tmp)) {
			get_e
				* value = findElement(tmp, true);
			return true;
		} else if (sscanf(tmp, "WEIGHT:%254s", tmp)) {
			get_e
				* value = getElement(e)->weight;
			return true;
		} else if (sscanf(tmp, "SPRAY:%254s", tmp)) {
			get_e
				* value = getElement(e)->spray;
			return true;
		} else if (sscanf(tmp, "SLIDE:%254s", tmp)) {
			get_e
				* value = getElement(e)->slide;
			return true;
		} else if (sscanf(tmp, "VISCOUSITY:%254s", tmp)) {
			get_e
				* value = getElement(e)->viscousity;
			return true;
		} else if (sscanf(tmp, "R:%254s", tmp)) {
			get_e
				* value = getElement(e)->r;
			return true;
		} else if (sscanf(tmp, "G:%254s", tmp)) {
			get_e
				* value = getElement(e)->g;
			return true;
		} else if (sscanf(tmp, "B:%254s", tmp)) {
			get_e
				* value = getElement(e)->b;
			return true;
		} else if (sscanf(tmp, "ICONTYPE:%254s", tmp)) {
			get_e
				* value = static_cast<int>(reinterpret_cast<intptr_t>(getElement(e)->icon->type));
			return true;
		} else if (sscanf(tmp, "ICONTEXT:%254s", tmp)) {
			get_e
				* value = static_cast<int>(reinterpret_cast<intptr_t>(getElement(e)->icon->text));
			return true;
		} else if (sscanf(tmp, "CUSTOMR1:%254s", tmp)) {
			get_e
				* value = getElement(e)->cr1;
			return true;
		} else if (sscanf(tmp, "CUSTOMG1:%254s", tmp)) {
			get_e
				* value = getElement(e)->cg1;
			return true;
		} else if (sscanf(tmp, "CUSTOMB1:%254s", tmp)) {
			get_e
				* value = getElement(e)->cb1;
			return true;
		} else if (sscanf(tmp, "CUSTOMR2:%254s", tmp)) {
			get_e
				* value = getElement(e)->cr2;
			return true;
		} else if (sscanf(tmp, "CUSTOMG2:%254s", tmp)) {
			get_e
				* value = getElement(e)->cg2;
			return true;
		} else if (sscanf(tmp, "CUSTOMB2:%254s", tmp)) {
			get_e
				* value = getElement(e)->cb2;
			return true;
		} else if (sscanf(tmp, "CUSTOMR3:%254s", tmp)) {
			get_e
				* value = getElement(e)->cr3;
			return true;
		} else if (sscanf(tmp, "CUSTOMG3:%254s", tmp)) {
			get_e
				* value = getElement(e)->cg3;
			return true;
		} else if (sscanf(tmp, "CUSTOMB3:%254s", tmp)) {
			get_e
				* value = getElement(e)->cb3;
			return true;
		} else if (sscanf(tmp, "DEATHRATE:%254s", tmp)) {
			unsigned int i;
			if (sscanf(tmp, "%i:%254s", &i, tmp) == 2) {
				get_e
					if ((getElement(e)->dies)->empty()) {
						*value = 0;
						return false;
					}
				auto it2 = getElement(e)->dies->begin();
				if (i <= static_cast<unsigned int>((getElement(e)->dies)->size()))
					for (unsigned int ii = 0; ii < i; ii++) ++it2;
				*value = (*it2)->rate;
				return true;
			}
		}
	} else if (sscanf(name, "GROUP:%254s", tmp)) {
		int val = 0;
		const std::size_t tmplen = strlen(tmp);
		for (unsigned int i = 0; i < tmplen - 1; i++) {
			if (tmp[i] == ':') {
				tmp[i] = 0;
				getVar(tmp + i + 1, &val);
				*value = getGroupElement(tmp, val);
				return true;
			}
		}
		*value = findGroup(tmp, false, -1);
		return true;
	} else if (sscanf(name, "GROUPORDER:%254s", tmp)) {
		int val = 0;
		const std::size_t tmplen = strlen(tmp);
		for (unsigned int i = 0; i < tmplen - 1; i++) {
			if (tmp[i] == ':') {
				tmp[i] = 0;
				getVar(tmp + i + 1, &val);
				*value = getGroupElementOrder(tmp, val);
				return true;
			}
		}
		*value = 0;
		return false;
	} else if (sscanf(name, "COUNT:GROUP:%254s", tmp)) {
		int i = 0;
		if (!getVar(name + 12, &i)) getVar(name + 6, &i);
		if (getGroup(i))
			*value = static_cast<int>(getGroup(i)->elements.size());
		else
			*value = 0;
		return true;
	} else if (!strcmp(name, "COUNT:GROUPS")) {
		*value = countGroups();
		return true;
	} else if (sscanf(name, "%i", value)) {
		return true;
	}

	for (auto* v : vars) {
		if (!strcmp(v->name, name)) {
			*value = v->value;
			return true;
		}
	}
	*value = 0;
	return false;
}

Varint::Varint() {
	a = nullptr;
	b = nullptr;
	value = nullptr;
	fixedvalue = 0;
	ok = true;
	function = 0;
	varvalue = nullptr;
	text = new char[1];
	text[0] = 0;
	max = 32768;
	trigger = nullptr;
	params = nullptr;
}

Varint::Varint(char* v, int max, int f) {
	a = new Varint(v);
	b = nullptr;
	function = f;
	ok = true;
	varvalue = nullptr;
	value = v;
	text = new char[strlen(v) + 1];
	strcpy(text, v);
	this->max = max;
	trigger = nullptr;
	params = nullptr;
}

Varint::Varint(int v) {
	value = nullptr;
	fixedvalue = v;
	ok = true;
	function = 0;
	varvalue = nullptr;
	text = new char[20];
	snprintf(text, 20, "%i", v);
	max = 32768;
	a = nullptr;
	b = nullptr;
	trigger = nullptr;
	params = nullptr;
}

Varint::Varint(char* v, int max) {
	char* ov = v;
	a = nullptr;
	b = nullptr;
	varvalue = nullptr;
	this->max = max;
	value = nullptr;
	trigger = nullptr;
	params = nullptr;

	if ((v == nullptr) || (strlen(v) == 0)) {
		text = new char[1];
		text[0] = 0;
		fixedvalue = 0;
		ok = false;
		return;
	} else {
		text = new char[strlen(v) + 1];
		strcpy(text, v);
		while (*v == ' ')
			v++;
		if (*v == 0) {
			value = nullptr;
			ok = true;
			delete[] ov;
			return;
		}
		function = 0;
		int brace = 0;
		if (v[0] == '(') brace = 1;
		bool closebrace = false;
		if (v[strlen(v) - 1] == ')') {
			v[strlen(v) - 1] = 0;
			closebrace = true;
		}

		Token tokens(v + brace);
		char* t = tokens.getToken();
		if (!strcmp(t, "SIN") || !strcmp(t, "COS") || !strcmp(t, "SQRT") || !strcmp(t, "TAN") ||
			!strcmp(t, "SIN-1") || !strcmp(t, "COS-1") || !strcmp(t, "SQRT-1")) {
			char* t2 = tokens.getToken();
			if (!strcmp(t, "SIN")) { a = new Varint(t2, max, 100); function = 1; } else if (!strcmp(t, "COS")) { a = new Varint(t2, max, 101); function = 1; } else if (!strcmp(t, "SQRT")) { a = new Varint(t2, max, 102); function = 1; } else if (!strcmp(t, "TAN")) { a = new Varint(t2, max, 103); function = 1; } else if (!strcmp(t, "SIN-1")) { a = new Varint(t2, max, 104); function = 1; } else if (!strcmp(t, "COS-1")) { a = new Varint(t2, max, 105); function = 1; } else if (!strcmp(t, "TAN-1")) { a = new Varint(t2, max, 106); function = 1; }
		} else {
			if ((t[0] == '*') && (strlen(t) > 1)) {
				trigger = static_cast<void*>(findTrigger(t + 1, 0));
				params = getParameters(&tokens);
				ok = true;
				delete[] t;
				delete[] v;
				return;
			}
		}
		delete[] t;

		char* t2;
		char* v2;
		if (a) {
			t2 = tokens.getToken(true);
			v2 = tokens.getRest();
		} else {
			tokens.reset();
			t = tokens.getuntillast();
			if (!t) {
				tokens.reset();
				t = tokens.getToken(true);
			}
			t2 = tokens.getToken(true);
			v2 = tokens.getRest();
		}

		if (t2 && v2) {
			if (!a) a = new Varint(t);
			b = new Varint(v2);
			if (!strcmp(t2, "+"))      function = 2;
			else if (!strcmp(t2, "-"))      function = 3;
			else if (!strcmp(t2, "*"))      function = 4;
			else if (!strcmp(t2, "/"))      function = 5;
			else if (!strcmp(t2, "%"))      function = 6;
			else if (!strcmp(t2, "&"))      function = 7;
			else if (!strcmp(t2, "|"))      function = 8;
			else if (!strcmp(t2, "&&"))     function = 9;
			else if (!strcmp(t2, "||"))     function = 10;
			else if (!strcmp(t2, "!="))     function = 11;
			else if (!strcmp(t2, "=="))     function = 12;
			else if (!strcmp(t2, "<"))      function = 13;
			else if (!strcmp(t2, "<="))     function = 14;
			else if (!strcmp(t2, ">"))      function = 15;
			else if (!strcmp(t2, ">="))     function = 16;
			else if (!strcmp(t2, "RAND"))   function = 17;
			else if (!strcmp(t2, "PIXEL"))  function = 18;
			else if (!strcmp(t2, "INGROUP"))function = 19;
			else if (!strcmp(t2, "MIN"))    function = 20;
			else if (!strcmp(t2, "MAX"))    function = 21;
			else if (!strcmp(t2, "^"))      function = 22;
			else {
				ok = false;
				delete[] t2;
				delete[] ov;
				return;
			}
			ok = true;
			delete[] t2;
			delete[] ov;
			return;
		}

		if (function) {
			value = nullptr;
		} else {
			char tmp[255];
			if (t[0] == '$') {
				int i = t[1] - '0';
				if ((i >= 0) && (i < 10))
					function = 200 + i;
				value = t;
			} else if (t[strlen(t) - 1] == '%') {
				float d;
				sscanf(t, "%f", &d);
				function = 0;
				value = nullptr;
				fixedvalue = static_cast<int>(d * max / 100);
			} else if (sscanf(t, "%i", &fixedvalue)) {
				delete[] t;
				value = nullptr;
				function = 0;
			} else if (sscanf(t, "ELEMENT%254s", tmp) || sscanf(t, "GROUP:%254s", tmp)
				|| sscanf(t, "COUNT:GROUP:%254s", tmp) || sscanf(t, "COUNT:GROUPS%254s", tmp)) {
				delete[] t;
				if (closebrace) v[strlen(v)] = ')';
				value = new char[strlen(v) + 1];
				strcpy(value, v);
			} else if (strchr(t, '[') && (strchr(t, '[') < strchr(t, ']'))) {
				*(strchr(t, ']')) = 0;
				// FIX: original allocated strlen(strchr(t,'[')) which is one byte short of null term
				const char* bracket_start = strchr(t, '[') + 1;
				char* tmp2 = new char[strlen(bracket_start) + 1];
				strcpy(tmp2, bracket_start);
				b = new Varint(tmp2);
				*(strchr(t, '[')) = 0;
				a = new Varint(t);
				function = 300;
			} else {
				for (auto* vp : vars) {
					if (!strcmp(vp->name, t))
						varvalue = vp;
				}
				if (varvalue == nullptr) {
					varvalue = new Var;
					strcpy(varvalue->name, t);
					varvalue->value = 0;
					vars.push_back(varvalue);
				}
				value = t;
			}
		}
	}
	ok = true;
	delete[] ov;
}

Varint::~Varint() {
	delete a;
	delete b;
	delete[] text;   // FIX: text is always allocated with new char[]
	delete[] value;  // FIX: value is allocated with new char[] when set
}

int Varint::val() {
	static Varint* debug = nullptr;
	static int tmp1 = 0, tmp2 = 0, tmp3 = 0;

	if (debugvar->value && (debug != this) && text) {
		char* tmp = new char[512 + strlen(text)];
		snprintf(tmp, 512 + strlen(text), "calculating var: %s", text);
		std::cout << tmp << std::endl;
		debug = this;
		int v = val();
		debug = nullptr;
		snprintf(tmp, 512 + strlen(text), "calculated  var: %s = %i", text, v);
		std::cout << tmp << std::endl;
		delete[] tmp;
		return v;
	}

	if (!ok) return 0;

	if (function) {
		if ((function > 199) && (function < 210)) {
			if ((parameterpos >= 0) && parameters[parameterpos])
				return parameters[parameterpos][function - 200];
			return 0;
		}
		switch (function) {
		case 1:
			return a->val();
		case 2:
			return a->val() + b->val();
		case 3:
			return a->val() - b->val();
		case 4:
			return a->val() * b->val();
		case 5:
			if (b->val()) return a->val() / b->val();
			return 0;
		case 6:
			return a->val() % b->val();
		case 7:
			return a->val() & b->val();
		case 8:
			return a->val() | b->val();
		case 9:
			return a->val() && b->val();
		case 10:
			return a->val() || b->val();
		case 11:
			return a->val() != b->val();
		case 12:
			return a->val() == b->val();
		case 13:
			return a->val() < b->val();
		case 14:
			return a->val() <= b->val();
		case 15:
			return a->val() > b->val();
		case 16:
			return a->val() >= b->val();
		case 17:
			return RANDOMNUMBER * (b->val() - a->val()) / 32768 + a->val();
		case 18:
			return getPixel(a->val(), b->val());
		case 19:
			return isElementInGroup(getGroup(b->val()), a->val());
		case 20:
			tmp1 = a->val();
			tmp2 = b->val();
			return (tmp1 < tmp2) ? tmp1 : tmp2;
		case 21:
			tmp1 = a->val();
			tmp2 = b->val();
			return (tmp1 < tmp2) ? tmp2 : tmp1;
		case 22:
			tmp1 = a->val();
			tmp2 = b->val();
			tmp3 = 1;
			for (int i = 0; i < tmp2; i++) tmp3 *= tmp1;
			return tmp3;
		case 100:
			return static_cast<int>(std::sin(PI * a->val() / 180000.0) * 1000.0);
		case 101:
			return static_cast<int>(std::cos(PI * a->val() / 180000.0) * 1000.0);
		case 102:
			return static_cast<int>(std::sqrt(static_cast<double>(a->val())));
		case 103:
			return static_cast<int>(std::tan(PI * a->val() / 180000.0) * 1000.0);
		case 104:
			return static_cast<int>(std::asin(a->val() / 1000.0) * 180000.0 / PI);
		case 105:
			return static_cast<int>(std::acos(a->val() / 1000.0) * 180000.0 / PI);
		case 106:
			return static_cast<int>(std::atan(a->val() / 1000.0) * 180000.0 / PI);
		case 300: {
			char tmp[255];
			snprintf(tmp, sizeof(tmp), "%s%i", a->text, b->val());
			int i = 0;
			getVar(tmp, &i);
			return i;
		}
		default:
			break;
		}
	}

	if (trigger) {
		if (params) addparams(calcparams(params));
		int i = (static_cast<Trigger*>(trigger))->exec();
		if (params) removeparams();
		setreturn = false;
		return i;
	}

	if (value == nullptr) return fixedvalue;
	if (varvalue) return varvalue->value;

	int r = 0;
	if (sscanf(value, "%i", &r)) return r;
	else getVar(value, &r);
	return r;
}