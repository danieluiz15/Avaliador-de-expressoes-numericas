#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "expressao.h"

typedef struct {
	char **data;
	int top;
	int cap;
} StrStack;

static StrStack *stack_new(int cap) {
	StrStack *s = malloc(sizeof(StrStack));
	s->cap = cap > 0 ? cap : 16;
	s->top = 0;
	s->data = malloc(sizeof(char*) * s->cap);
	return s;
}

static void stack_free(StrStack *s) {
	if (!s) return;
	for (int i = 0; i < s->top; ++i) free(s->data[i]);
	free(s->data);
	free(s);
}

static void stack_grow(StrStack *s) {
	s->cap *= 2;
	s->data = realloc(s->data, sizeof(char*) * s->cap);
}

static void stack_push(StrStack *s, const char *tok) {
	if (s->top >= s->cap) stack_grow(s);
	s->data[s->top++] = strdup(tok);
}

static char *stack_pop(StrStack *s) {
	if (s->top == 0) return NULL;
	return s->data[--s->top];
}

static char *stack_peek(StrStack *s) {
	if (s->top == 0) return NULL;
	return s->data[s->top-1];
}

static int is_operator_char(char c) {
	return c == '+' || c == '-' || c == '*' || c == '/' || c == '^' || c == '%';
}

static int precedence(const char *op) {
	if (!op) return -1;
	if (strcmp(op, "+") == 0 || strcmp(op, "-") == 0) return 1;
	if (strcmp(op, "*") == 0 || strcmp(op, "/") == 0 || strcmp(op, "%") == 0) return 2;
	if (strcmp(op, "^") == 0) return 3;
	return 4;
}

static int is_right_assoc(const char *op) {
	if (!op) return 0;
	return strcmp(op, "^") == 0;
}

char * getFormaPosFixa(char *StrInFixa) {
	if (!StrInFixa) return NULL;

	const char *s = StrInFixa;
	size_t out_cap = 256;
	size_t out_len = 0;
	char *out = malloc(out_cap);
	out[0] = '\0';

	StrStack *opstack = stack_new(32);

	for (size_t i = 0; s[i] != '\0';) {
		if (isspace((unsigned char)s[i])) { i++; continue; }

		// Sequência de dígitos (número)
		if (isdigit((unsigned char)s[i]) || s[i] == '.') {
			size_t j = i;
			while (isdigit((unsigned char)s[j]) || s[j] == '.') j++;
			size_t len = j - i;
			if (out_len + len + 2 >= out_cap) {
				out_cap = (out_len + len + 2) * 2;
				out = realloc(out, out_cap);
			}
			memcpy(out + out_len, s + i, len);
			out_len += len;
			out[out_len++] = ' ';
			out[out_len] = '\0';
			i = j;
			continue;
		}

		// Identificadores (variáveis ou funções)
		if (isalpha((unsigned char)s[i])) {
			size_t j = i;
			while (isalpha((unsigned char)s[j])) j++;
			size_t len = j - i;
			char *name = malloc(len+1);
			memcpy(name, s + i, len);
			name[len] = '\0';

			// Se for uma função seguida por '(' empilha como função, se não
			//   trata como variável e envia para a saída 
			size_t k = j;
			while (isspace((unsigned char)s[k])) k++;
			if (s[k] == '(') {
				stack_push(opstack, name);
				free(name);
			} else {
				if (out_len + len + 2 >= out_cap) {
					out_cap = (out_len + len + 2) * 2;
					out = realloc(out, out_cap);
				}
				memcpy(out + out_len, name, len);
				out_len += len;
				out[out_len++] = ' ';
				out[out_len] = '\0';
				free(name);
			}
			i = j;
			continue;
		}

		char c = s[i];
		if (c == '(') {
			char t[2] = "(";
			stack_push(opstack, t);
			i++; continue;
		}

		if (c == ')') {
			// Desempilha até encontrar '('
			char *top;
			while ((top = stack_pop(opstack)) != NULL) {
				if (strcmp(top, "(") == 0) { free(top); break; }
				size_t len = strlen(top);
				if (out_len + len + 2 >= out_cap) {
					out_cap = (out_len + len + 2) * 2;
					out = realloc(out, out_cap);
				}
				memcpy(out + out_len, top, len);
				out_len += len;
				out[out_len++] = ' ';
				out[out_len] = '\0';
				free(top);
			}
			// Se tiver uma função no topo da pilha, coloca ela na saída
			char *peek = stack_peek(opstack);
			if (peek && isalpha((unsigned char)peek[0])) {
				char *fn = stack_pop(opstack);
				size_t len = strlen(fn);
				if (out_len + len + 2 >= out_cap) {
					out_cap = (out_len + len + 2) * 2;
					out = realloc(out, out_cap);
				}
				memcpy(out + out_len, fn, len);
				out_len += len;
				out[out_len++] = ' ';
				out[out_len] = '\0';
				free(fn);
			}
			i++; continue;
		}

		// Operadores 
		if (is_operator_char(c)) {
			char cur[2] = {c, '\0'};
			char *top = stack_peek(opstack);
			while (top && (isalpha((unsigned char)top[0]) || is_operator_char(top[0]))) {
				int ptop = precedence(top);
				int pcur = precedence(cur);
				if ( (ptop > pcur) || (ptop == pcur && !is_right_assoc(cur)) ) {
					char *pop = stack_pop(opstack);
					size_t len = strlen(pop);
					if (out_len + len + 2 >= out_cap) {
						out_cap = (out_len + len + 2) * 2;
						out = realloc(out, out_cap);
					}
					memcpy(out + out_len, pop, len);
					out_len += len;
					out[out_len++] = ' ';
					out[out_len] = '\0';
					free(pop);
					top = stack_peek(opstack);
				} else break;
			}
			stack_push(opstack, cur);
			i++; continue;
		}

		i++;
	}

	// Desempilha os operadores restantes 
	char *t;
	while ((t = stack_pop(opstack)) != NULL) {
		if (strcmp(t, "(") == 0 || strcmp(t, ")") == 0) { free(t); continue; }
		size_t len = strlen(t);
		if (out_len + len + 2 >= out_cap) {
			out_cap = (out_len + len + 2) * 2;
			out = realloc(out, out_cap);
		}
		memcpy(out + out_len, t, len);
		out_len += len;
		out[out_len++] = ' ';
		out[out_len] = '\0';
		free(t);
	}

	// Remove o espaço final, se existir
	if (out_len > 0 && out[out_len-1] == ' ') out[out_len-1] = '\0';

	stack_free(opstack);
	return out;
}

char * getFormaInFixa(char *StrPosFixa) {
	if (!StrPosFixa) return NULL;

	char *copy = strdup(StrPosFixa);
	if (!copy) return NULL;

	// Pilha de expressões
	typedef struct { char **data; int *prec; int top; int cap; } ExprStack;
	ExprStack stk;
	stk.cap = 64; stk.top = 0;
	stk.data = malloc(sizeof(char*) * stk.cap);
	stk.prec = malloc(sizeof(int) * stk.cap);
	if (!stk.data || !stk.prec) { free(copy); if (stk.data) free(stk.data); if (stk.prec) free(stk.prec); return NULL; }

	char *saveptr = NULL;
	char *tok = strtok_s(copy, " ", &saveptr);

	// Lista de funções unárias 
	const char *funcoes[] = {"sin","sen","cos","tan","tg","log","ln","exp","sqrt","raiz", NULL};

	auto_expr_push:
	while (tok) {
		// Operador binário
		if (tok[1] == '\0' && is_operator_char(tok[0])) {
			if (stk.top < 2) { goto err; }
			char *right = stk.data[--stk.top]; int rprec = stk.prec[stk.top];
			char *left = stk.data[--stk.top]; int lprec = stk.prec[stk.top];

			int pcur = precedence(tok);
			int rightNeeds = 0, leftNeeds = 0;
			int rightAssoc = is_right_assoc(tok);
			if (rightAssoc) {
				leftNeeds = (lprec <= pcur);
				rightNeeds = (rprec < pcur);
			} else {
				leftNeeds = (lprec < pcur);
				rightNeeds = (rprec <= pcur);
			}

			size_t newlen = strlen(left) + strlen(right) + 2 + (leftNeeds?2:0) + (rightNeeds?2:0);
			char *expr = malloc(newlen + 1);
			char *p = expr;
			if (leftNeeds) *p++ = '(';
			memcpy(p, left, strlen(left)); p += strlen(left);
			if (leftNeeds) *p++ = ')';
			*p++ = tok[0];
			if (rightNeeds) *p++ = '(';
			memcpy(p, right, strlen(right)); p += strlen(right);
			if (rightNeeds) *p++ = ')';
			*p = '\0';

			free(left); free(right);

			if (stk.top + 1 > stk.cap) {
				stk.cap *= 2;
				stk.data = realloc(stk.data, sizeof(char*) * stk.cap);
				stk.prec = realloc(stk.prec, sizeof(int) * stk.cap);
			}
			stk.data[stk.top] = expr;
			stk.prec[stk.top] = pcur;
			stk.top++;

			tok = strtok_s(NULL, " ", &saveptr);
			continue;
		}

		// Verifica se é função unária 
		int is_fn = 0; int fn_index = -1;
		for (int i = 0; funcoes[i] != NULL; ++i) {
			if (strcmp(tok, funcoes[i]) == 0) { is_fn = 1; fn_index = i; break; }
		}
		if (is_fn) {
			if (stk.top < 1) goto err;
			char *arg = stk.data[--stk.top]; int aparent = stk.prec[stk.top];
			size_t newlen = strlen(tok) + 2 + strlen(arg);
			char *expr = malloc(newlen + 1);
			sprintf(expr, "%s(%s)", tok, arg);
			free(arg);
			int pcur = precedence(tok);
			if (stk.top + 1 > stk.cap) {
				stk.cap *= 2;
				stk.data = realloc(stk.data, sizeof(char*) * stk.cap);
				stk.prec = realloc(stk.prec, sizeof(int) * stk.cap);
			}
			stk.data[stk.top] = expr;
			stk.prec[stk.top] = pcur;
			stk.top++;
			tok = strtok_s(NULL, " ", &saveptr);
			continue;
		}

		// Número ou variável 
		if (tok[0] != '\0') {
			char *atom = strdup(tok);
			if (!atom) goto err;
			if (stk.top + 1 > stk.cap) {
				stk.cap *= 2;
				stk.data = realloc(stk.data, sizeof(char*) * stk.cap);
				stk.prec = realloc(stk.prec, sizeof(int) * stk.cap);
			}
			stk.data[stk.top] = atom;
			stk.prec[stk.top] = 100; 
			stk.top++;
			tok = strtok_s(NULL, " ", &saveptr);
			continue;
		}
	}

	// Resultado final
	if (stk.top != 1) goto err;
	char *res = strdup(stk.data[0]);
	// Libera pilha
	for (int i = 0; i < stk.top; ++i) free(stk.data[i]);
	free(stk.data); free(stk.prec); free(copy);
	return res;

err:
	// Limpa e retorna NULL em caso de erro
	for (int i = 0; i < stk.top; ++i) free(stk.data[i]);
	free(stk.data); free(stk.prec); free(copy);
	return NULL;
}

float getValorPosFixa(char *StrPosFixa) {
	if (!StrPosFixa) return NAN;

	// Pilha de valores 
	typedef struct { double *data; int top; int cap; } ValStack;
	ValStack stk;
	stk.cap = 64; stk.top = 0; stk.data = malloc(sizeof(double) * stk.cap);
	if (!stk.data) return NAN;

	char *copy = strdup(StrPosFixa);
	if (!copy) { free(stk.data); return NAN; }
	char *saveptr = NULL;
	char *tok = strtok_s(copy, " ", &saveptr);

	while (tok) {
		if (tok[1] == '\0' && is_operator_char(tok[0])) {
			if (stk.top < 2) { free(copy); free(stk.data); return NAN; }
			double b = stk.data[--stk.top];
			double a = stk.data[--stk.top];
			double res = 0.0;
			switch (tok[0]) {
				case '+': res = a + b; break;
				case '-': res = a - b; break;
				case '*': res = a * b; break;
				case '/': res = b == 0.0 ? NAN : a / b; break;
				case '%': res = b == 0.0 ? NAN : fmod(a, b); break;
				case '^': res = pow(a, b); break;
				default: res = NAN; break;
			}
			if (stk.top >= stk.cap) {}
			if (stk.top + 1 > stk.cap) {
				stk.cap *= 2; stk.data = realloc(stk.data, sizeof(double) * stk.cap);
			}
			stk.data[stk.top++] = res;
			tok = strtok_s(NULL, " ", &saveptr);
			continue;
		}
		// Função unária
		if (strcmp(tok, "sin") == 0 || strcmp(tok, "sen") == 0 || strcmp(tok, "cos") == 0 ||
			strcmp(tok, "tan") == 0 || strcmp(tok, "tg") == 0 || strcmp(tok, "log") == 0 ||
			strcmp(tok, "sqrt") == 0 || strcmp(tok, "raiz") == 0) {
			if (stk.top < 1) { free(copy); free(stk.data); return NAN; }
			double a = stk.data[--stk.top];
			double res = NAN;

			// Converte graus para radianos
			if (strcmp(tok, "sin") == 0 || strcmp(tok, "sen") == 0) {
				double rad = a * acos(-1.0) / 180.0;
				res = sin(rad);
			} else if (strcmp(tok, "cos") == 0) {
				double rad = a * acos(-1.0) / 180.0;
				res = cos(rad);
			} else if (strcmp(tok, "tan") == 0 || strcmp(tok, "tg") == 0) {
				double rad = a * acos(-1.0) / 180.0;
				res = tan(rad);
			} else if (strcmp(tok, "log") == 0) {
				res = log10(a); 
			} else if (strcmp(tok, "sqrt") == 0 || strcmp(tok, "raiz") == 0) {
				res = sqrt(a);
			}
			if (stk.top + 1 > stk.cap) {
				stk.cap *= 2; stk.data = realloc(stk.data, sizeof(double) * stk.cap);
			}
			stk.data[stk.top++] = res;
			tok = strtok_s(NULL, " ", &saveptr);
			continue;
		}

		char *endp = NULL;
		double v = strtod(tok, &endp);
		if (endp != tok) {
			if (stk.top + 1 > stk.cap) { stk.cap *= 2; stk.data = realloc(stk.data, sizeof(double) * stk.cap); }
			stk.data[stk.top++] = v;
		} else {
			// Token inválido
			free(copy); free(stk.data); return NAN;
		}

		tok = strtok_s(NULL, " ", &saveptr);
	}

	double result = NAN;
	if (stk.top == 1) result = stk.data[0];

	free(copy);
	free(stk.data);
	return (float)result;
}

