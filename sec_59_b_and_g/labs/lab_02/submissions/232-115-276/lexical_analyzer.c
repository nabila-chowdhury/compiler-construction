#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define MAXLEX 256

static int is_keyword(const char *s) {
    const char *kw[] = {"int", "if", "else", "while", "return"};
    for (int i = 0; i < 5; i++)
        if (strcmp(s, kw[i]) == 0) return 1;
    return 0;
}

static void emit(const char *lexeme, const char *token) {
    printf("%s,%s\n", lexeme, token);
}

/* For = ! < > : checks if the next character is '=' to make a 2-char operator.
   Returns the next character to process. */
static int with_equal(FILE *fp, const char *two, const char *two_tok,
                      const char *one, const char *one_tok) {
    int next = fgetc(fp);
    if (next == '=') {
        emit(two, two_tok);
        return fgetc(fp);
    }
    emit(one, one_tok);
    return next;
}

/* For & and | : only valid when doubled (&& or ||), otherwise UNKNOWN. */
static int doubled(FILE *fp, int c, const char *two) {
    int next = fgetc(fp);
    if (next == c) {
        emit(two, "LOGICAL_OPERATOR");
        return fgetc(fp);
    }
    char one[2] = {(char)c, '\0'};
    emit(one, "UNKNOWN");
    return next;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
        return 1;
    }
    FILE *fp = fopen(argv[1], "r");
    if (fp == NULL) {
        fprintf(stderr, "Error: cannot open file '%s'\n", argv[1]);
        return 1;
    }

    int c = fgetc(fp);
    while (c != EOF) {
        if (isspace(c)) {
            c = fgetc(fp);
        }
        else if (isalpha(c) || c == '_') {
            char buf[MAXLEX];
            int n = 0;
            while (c != EOF && (isalnum(c) || c == '_')) {
                if (n < MAXLEX - 1) buf[n++] = (char)c;
                c = fgetc(fp);
            }
            buf[n] = '\0';
            emit(buf, is_keyword(buf) ? "KEYWORD" : "IDENTIFIER");
        }
        else if (isdigit(c)) {
            char buf[MAXLEX];
            int n = 0;
            while (c != EOF && isdigit(c)) {
                if (n < MAXLEX - 1) buf[n++] = (char)c;
                c = fgetc(fp);
            }
            buf[n] = '\0';
            emit(buf, "INTEGER");
        }
        else if (c == '/') {
            int next = fgetc(fp);
            if (next == '/') {                      /* line comment */
                c = fgetc(fp);
                while (c != EOF && c != '\n') c = fgetc(fp);
            } else if (next == '*') {               /* block comment */
                int prev = 0;
                c = fgetc(fp);
                while (c != EOF && !(prev == '*' && c == '/')) {
                    prev = c;
                    c = fgetc(fp);
                }
                if (c == EOF) {
                    fprintf(stderr, "Error: unclosed block comment\n");
                    fclose(fp);
                    return 1;
                }
                c = fgetc(fp);
            } else {                                /* division */
                emit("/", "ARITHMETIC_OPERATOR");
                c = next;
            }
        }
        else if (c == '=') c = with_equal(fp, "==", "RELATIONAL_OPERATOR", "=", "ASSIGNMENT_OPERATOR");
        else if (c == '!') c = with_equal(fp, "!=", "RELATIONAL_OPERATOR", "!", "LOGICAL_OPERATOR");
        else if (c == '<') c = with_equal(fp, "<=", "RELATIONAL_OPERATOR", "<", "RELATIONAL_OPERATOR");
        else if (c == '>') c = with_equal(fp, ">=", "RELATIONAL_OPERATOR", ">", "RELATIONAL_OPERATOR");
        else if (c == '&') c = doubled(fp, c, "&&");
        else if (c == '|') c = doubled(fp, c, "||");
        else {
            char one[2] = {(char)c, '\0'};
            if (strchr("+-*%", c))
                emit(one, "ARITHMETIC_OPERATOR");
            else if (strchr("(){}[];,", c))
                emit(one, "DELIMITER");
            else
                emit(one, "UNKNOWN");
            c = fgetc(fp);
        }
    }

    fclose(fp);
    return 0;
}