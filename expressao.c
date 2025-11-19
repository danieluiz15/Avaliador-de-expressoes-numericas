#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "expressao.h"

/*
 * Conversor de expressão infixa para pós-fixa (notação RPN) usando o
 * algoritmo shunting-yard de Dijkstra.
 *
 * Regras suportadas:
 * - Operadores: + - * / ^
 * - Parênteses: ( )
 * - Números: inteiros e com ponto decimal (ex.: 12, 3.14)
 * - Funções/identificadores alfabéticos: sin, cos, log, etc.
 *   Se um identificador for seguido de '(', ele é tratado como função.
 * - Variáveis simples (identificadores não seguidos de '(') são emitidas
 *   na saída como tokens.
 *
 * A função retorna uma string alocada com malloc contendo os tokens da
 * expressão pós-fixa separados por espaços. O chamador deve liberar a
 * memória com free().
 */

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
	/* Agora reconhece também '%' (módulo) */
	return c == '+' || c == '-' || c == '*' || c == '/' || c == '^' || c == '%';
}

/* Precedência de operadores: maior valor = maior precedência */
static int precedence(const char *op) {
	if (!op) return -1;
	if (strcmp(op, "+") == 0 || strcmp(op, "-") == 0) return 1;
	if (strcmp(op, "*") == 0 || strcmp(op, "/") == 0 || strcmp(op, "%") == 0) return 2;
	if (strcmp(op, "^") == 0) return 3;
	/* funções identificadores tratados como maior precedência para efeito
	   de desempilhamento quando necessário */
	return 4;
}

/* '^' é associativo à direita */
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

		/* NÚMEROS: sequência de dígitos possivelmente com '.' */
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

		/* IDENTIFICADORES: letras (funções ou variáveis) */
		if (isalpha((unsigned char)s[i])) {
			size_t j = i;
			while (isalpha((unsigned char)s[j])) j++;
			size_t len = j - i;
			char *name = malloc(len+1);
			memcpy(name, s + i, len);
			name[len] = '\0';

			/* Se for função (seguida por '('), empilha como função; caso contrário
			   trata como variável e envia para a saída */
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
			/* Desempilha até encontrar '(' */
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
			/* Se houver uma função no topo da pilha, coloca-a na saída */
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

		/* OPERADORES */
		if (is_operator_char(c)) {
			char cur[2] = {c, '\0'};
			char *top = stack_peek(opstack);
			while (top && (isalpha((unsigned char)top[0]) || is_operator_char(top[0]))) {
				int ptop = precedence(top);
				int pcur = precedence(cur);
				if ( (ptop > pcur) || (ptop == pcur && !is_right_assoc(cur)) ) {
					/* pop top para saída */
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

		/* Caracter inesperado: ignora e avança */
		i++;
	}

	/* Desempilha operadores restantes */
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

	/* Remove espaço final, se existir */
	if (out_len > 0 && out[out_len-1] == ' ') out[out_len-1] = '\0';

	stack_free(opstack);
	return out;
}

/*
 * Converte expressão em notação pós-fixa (tokens separados por espaço)
 * para notação infixa. Retorna uma string alocada com malloc contendo
 * a expressão infixa (com parênteses para preservar a precedência).
 * O chamador deve liberar a string retornada com free().
 *
 * Estratégia:
 * - Usamos uma pilha de strings (operandos).
 * - Para cada token:
 *   - se for operador binário, desempilha dois operandos (direito, esquerdo),
 *     monta "(esquerdo op direito)" e empilha de volta;
 *   - se for uma função conhecida, desempilha um operando e empilha "fn(oper)";
 *   - caso contrário (número ou variável), empilha o token tal qual.
 *
 * Observação: para distinguir funções de variáveis usamos uma lista simples de
 * funções matemáticas comuns (sin, cos, tan, log, ln, exp, sqrt). Identificadores
 * que não estiverem nessa lista serão tratados como variáveis/operando.
 */
char * getFormaInFixa(char *StrPosFixa) {
	if (!StrPosFixa) return NULL;

	/* cria cópia porque strtok modifica a string */
	char *copy = strdup(StrPosFixa);
	if (!copy) return NULL;

	StrStack *stk = stack_new(64);
	char *saveptr = NULL;
	char *tok = strtok_s(copy, " ", &saveptr);

	 /* lista simples de funções conhecidas (inclui nomes em português)
		 para que getFormaInFixa reconstrua corretamente funções como sen, tg, raiz */
	 const char *funcoes[] = {"sin","sen","cos","tan","tg","log","ln","exp","sqrt","raiz", NULL};

	while (tok) {
		/* operador binário (token de um caractere) */
		if (tok[1] == '\0' && is_operator_char(tok[0])) {
			char *b = stack_pop(stk);
			char *a = stack_pop(stk);
			if (!a || !b) {
				/* sintaxe inválida: empilha token literal e continua */
				if (a) stack_push(stk, a), free(a);
				if (b) stack_push(stk, b), free(b);
				stack_push(stk, tok);
				tok = strtok_s(NULL, " ", &saveptr);
				continue;
			}
			size_t len = strlen(a) + strlen(b) + 4; /* espaço para op e parênteses */
			char *expr = malloc(len + strlen(tok) + 1);
			sprintf(expr, "(%s %s %s)", a, tok, b);
			free(a); free(b);
			stack_push(stk, expr);
			free(expr);
		} else {
			/* verifica se é uma função conhecida */
			int is_fn = 0;
			for (int i = 0; funcoes[i] != NULL; ++i) {
				if (strcmp(tok, funcoes[i]) == 0) { is_fn = 1; break; }
			}
			if (is_fn) {
				char *a = stack_pop(stk);
				if (!a) {
					/* sem operandos: empilha token literal */
					stack_push(stk, tok);
				} else {
					size_t len = strlen(tok) + 2 + strlen(a) + 1; /* fn( a ) */
					char *expr = malloc(len + 1);
					sprintf(expr, "%s(%s)", tok, a);
					free(a);
					stack_push(stk, expr);
					free(expr);
				}
			} else {
				/* número ou variável: empilha como está */
				stack_push(stk, tok);
			}
		}

		tok = strtok_s(NULL, " ", &saveptr);
	}

	/* resultado final deve ser o único item da pilha */
	char *res = stack_pop(stk);
	if (!res) {
		/* expressão vazia ou sintaxe inválida */
		stack_free(stk);
		free(copy);
		return NULL;
	}

	/* se ainda houver itens na pilha, não tentamos concatenar — liberamos o restante */
	stack_free(stk);

	/* res já foi alocada via strdup/strdup-like; devolvemos-a (caller libera) */
	char *ret = strdup(res);
	free(res);
	free(copy);
	return ret;
}

/*
 * Avalia uma expressão em notação pós-fixa (tokens separados por espaço)
 * e retorna o valor numérico (float). Em caso de erro sintático retorna NAN.
 * Suporta operadores binários: + - * / ^
 * Suporta funções unárias: sin, cos, tan, log (base 10), ln (natural), exp, sqrt
 */
float getValorPosFixa(char *StrPosFixa) {
	if (!StrPosFixa) return NAN;

	/* pilha de valores (double para maior precisão interna) */
	typedef struct { double *data; int top; int cap; } ValStack;
	ValStack stk;
	stk.cap = 64; stk.top = 0; stk.data = malloc(sizeof(double) * stk.cap);
	if (!stk.data) return NAN;

	char *copy = strdup(StrPosFixa);
	if (!copy) { free(stk.data); return NAN; }
	char *saveptr = NULL;
	char *tok = strtok_s(copy, " ", &saveptr);

	while (tok) {
		/* operador binário de um caractere */
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
			if (stk.top >= stk.cap) { /* never happens here */ }
			/* push */
			if (stk.top + 1 > stk.cap) {
				stk.cap *= 2; stk.data = realloc(stk.data, sizeof(double) * stk.cap);
			}
			stk.data[stk.top++] = res;
			tok = strtok_s(NULL, " ", &saveptr);
			continue;
		}

		/* funções unárias (suportadas em inglês e português)
		   - sen/sin, cos, tg/tan: argumentos em GRAUS (converter para radianos)
		   - log: log10 (decimal)
		   - ln: log natural
		   - exp: exponencial
		   - sqrt/raiz: raiz quadrada
		*/
		if (strcmp(tok, "sin") == 0 || strcmp(tok, "sen") == 0 || strcmp(tok, "cos") == 0 ||
			strcmp(tok, "tan") == 0 || strcmp(tok, "tg") == 0 || strcmp(tok, "log") == 0 ||
			strcmp(tok, "sqrt") == 0 || strcmp(tok, "raiz") == 0) {
			if (stk.top < 1) { free(copy); free(stk.data); return NAN; }
			double a = stk.data[--stk.top];
			double res = NAN;
			/* converter graus->radianos para trigonometria */
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

		/* número (tenta converter) */
		char *endp = NULL;
		double v = strtod(tok, &endp);
		if (endp != tok) {
			if (stk.top + 1 > stk.cap) { stk.cap *= 2; stk.data = realloc(stk.data, sizeof(double) * stk.cap); }
			stk.data[stk.top++] = v;
		} else {
			/* token desconhecido: sintaxe inválida */
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

