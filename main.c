#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "calculadora.h"

char * getFormaPosFixa(char *StrInFixa); // Retorna a forma posFixa de Str (inFixa)

int main(void) {
	char buf[512];

	for (;;) {
		printf("Selecione o modo:\n");
		printf("  1) Infixa -> Posfixa\n");
		printf("  2) Posfixa -> Infixa\n");
		printf("  s) Sair\n");
		printf("Escolha (1/2/s): ");
		if (!fgets(buf, sizeof(buf), stdin)) break;
		buf[strcspn(buf, "\r\n")] = '\0';
		if (strcmp(buf, "s") == 0 || strcmp(buf, "S") == 0) break;

		if (strcmp(buf, "1") == 0) {
			// Infixa -> Posfixa
			printf("Digite expressão infixa: ");
			if (!fgets(buf, sizeof(buf), stdin)) break;
			buf[strcspn(buf, "\r\n")] = '\0';
			if (strlen(buf) == 0) continue;

			char *posfixa = getFormaPosFixa(buf);
			if (posfixa) {
				printf("Posfixa: %s\n", posfixa);
				// Avalia a posfixa gerada
				float valor = getValorPosFixa(posfixa);
				if (!isnan(valor)) printf("Valor: %g\n\n", valor);
				else printf("Valor: erro ao avaliar expressão\n\n");
				free(posfixa);
			} else {
				printf("Erro: não foi possível converter a expressão.\n\n");
			}

		} else if (strcmp(buf, "2") == 0) {
			// Posfixa -> Infixa
			printf("Digite expressão posfixa: ");
			if (!fgets(buf, sizeof(buf), stdin)) break;
			buf[strcspn(buf, "\r\n")] = '\0';
			if (strlen(buf) == 0) continue;

			char *infixa = getFormaInFixa(buf);
			if (infixa) {
				printf("Infixa: %s\n", infixa);
				// Avalia a posfixa original (em buf)
                    float valor = getValorPosFixa(buf);
				if (!isnan(valor)) printf("Valor: %g\n\n", valor);
				else printf("Valor: erro ao avaliar expressão posfixa\n\n");
				free(infixa); 
			} else {
				printf("Erro: não foi possível converter a expressão posfixa.\n\n");
			}

		} else {
			printf("Opção inválida. Tente novamente.\n\n");
		}
	}

	printf("Programa encerrado!\n");    
	return 0;
}
