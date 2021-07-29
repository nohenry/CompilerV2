#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define NUM_CHARS 256

typedef struct trienode
{
    struct trienode *children[NUM_CHARS];
    bool terminal;
    int size;
    char val[32];
} trienode;
void print_trie(trienode *root);

trienode *create_node()
{
    trienode *newnode = malloc(sizeof(trienode));
    for (int i = 0; i < NUM_CHARS; i++)
    {
        newnode->children[i] = NULL;
    }

    newnode->terminal = false;
    newnode->size = 0;
    return newnode;
}

bool trie_insert(trienode **root, char *stext)
{
    if (*root == NULL)
    {
        *root = create_node();
    }
    unsigned char *text = (unsigned char *)stext;
    trienode *tmp = *root;
    int len = strlen(stext);
    for (int i = 0; i < len; i++)
    {
        if (tmp->children[text[i]] == NULL)
        {
            tmp->children[text[i]] = create_node();
            tmp->size++;
        }
        tmp = tmp->children[text[i]];
    }
    strcpy(tmp->val, stext);
    if (tmp->val[0] >= 'a' && tmp->val[0] <= 'z')
    {
        tmp->val[0] -= 32;
    }
    if (tmp->terminal)
    {
        return false;
    }
    else
    {
        return (tmp->terminal = true);
    }
}

void print_trie_rec(trienode *node, int n)
{
    if (node && node->size > 0)
    {
        for (int i = 0; i < NUM_CHARS; i++)
        {
            if (node->children[i])
            {
                for (int j = 0; j < n; j++)
                    printf("| ");
                printf("%c\n", i);
                print_trie_rec(node->children[i], n + 1);
            }
        }
    }
}

void print_trie(trienode *root)
{
    if (root == NULL)
    {
        printf("EMPTY TRIE\n");
        return;
    }
    print_trie_rec(root, 0);
}

FILE *out;

char *get_token_type(char *in)
{
    printf("%s\n", in);
    if (strcmp(in, "+") == 0)
    {
        return "Plus";
    }
    else if (strcmp(in, "++") == 0)
    {
        return "DoublePlus";
    }
    else if (strcmp(in, ";") == 0)
    {
        return "Semicolon";
    }
    else if (strcmp(in, "+=") == 0)
    {
        return "PlusEqual";
    }
    else if (strcmp(in, "-") == 0)
    {
        return "Minus";
    }
    else if (strcmp(in, "--") == 0)
    {
        return "DoubleMinus";
    }
    else if (strcmp(in, "-=") == 0)
    {
        return "MinusEqual";
    }
    else if (strcmp(in, "*") == 0)
    {
        return "Star";
    }
    else if (strcmp(in, "*=") == 0)
    {
        return "StarEqual";
    }
    else if (strcmp(in, "/") == 0)
    {
        return "ForwardSlash";
    }
    else if (strcmp(in, "/=") == 0)
    {
        return "SlashEqual";
    }
    else if (strcmp(in, "(") == 0)
    {
        return "LeftParen";
    }
    else if (strcmp(in, ")") == 0)
    {
        return "RightParen";
    }
    else if (strcmp(in, "{") == 0)
    {
        return "LeftCurly";
    }
    else if (strcmp(in, "}") == 0)
    {
        return "RightCurly";
    }
    else if (strcmp(in, "<") == 0)
    {
        return "LeftAngle";
    }
    else if (strcmp(in, ">") == 0)
    {
        return "RightAngle";
    }
    else if (strcmp(in, "[") == 0)
    {
        return "LeftSquare";
    }
    else if (strcmp(in, "]") == 0)
    {
        return "RightSquare";
    }
    else if (strcmp(in, ">=") == 0)
    {
        return "BiggerEqual";
    }
    else if (strcmp(in, "<=") == 0)
    {
        return "SmallerEqual";
    }
    else if (strcmp(in, "!>") == 0)
    {
        return "NotBigger";
    }
    else if (strcmp(in, "!<") == 0)
    {
        return "NotSmaller";
    }
    else if (strcmp(in, ",") == 0)
    {
        return "Comma";
    }
    else if (strcmp(in, "=") == 0)
    {
        return "Equal";
    }
    else if (strcmp(in, "==") == 0)
    {
        return "DoubleEqual";
    }
    else if (strcmp(in, "!=") == 0)
    {
        return "NotEqual";
    }
    else if (strcmp(in, ".") == 0)
    {
        return "Dot";
    }
    else if (strcmp(in, "..") == 0)
    {
        return "Spread";
    }
    else if (strcmp(in, "&") == 0)
    {
        return "Ampersand";
    }
    else if (strcmp(in, "&=") == 0)
    {
        return "AmpersandEquals";
    }
    else if (strcmp(in, "%") == 0)
    {
        return "Percent";
    }
    else if (strcmp(in, "%=") == 0)
    {
        return "PercentEqual";
    }
    else if (strcmp(in, "@") == 0)
    {
        return "At";
    }
    else if (strcmp(in, ":") == 0)
    {
        return "Colon";
    }
    else if (strcmp(in, "<<") == 0)
    {
        return "LeftShift";
    }
    else if (strcmp(in, ">>") == 0)
    {
        return "RightShift";
    }
    else if (strcmp(in, "<<>") == 0)
    {
        return "TripleLeftShift";
    }
    else if (strcmp(in, "<>>") == 0)
    {
        return "TripleRightShift";
    }
    else if (strcmp(in, "<<=") == 0)
    {
        return "LeftShiftEquals";
    }
    else if (strcmp(in, ">>=") == 0)
    {
        return "RightShiftEquals";
    }
    else if (strcmp(in, "<<>=") == 0)
    {
        return "TripleLeftShiftEquals";
    }
    else if (strcmp(in, "<>>=") == 0)
    {
        return "TripleRightShiftEquals";
    }
    else if (strcmp(in, "~") == 0)
    {
        return "Tilda";
    }
    else if (strcmp(in, "^") == 0)
    {
        return "Carrot";
    }
    else if (strcmp(in, "^=") == 0)
    {
        return "CarrotEquals";
    }
    else if (strcmp(in, "|") == 0)
    {
        return "Pipe";
    }
    else if (strcmp(in, "|=") == 0)
    {
        return "PipeEquals";
    }
    else if (strcmp(in, "!") == 0)
    {
        return "Not";
    }
    else if (strcmp(in, "=>") == 0)
    {
        return "FuncArrow";
    }
    else
    {
        return in;
    }
}

void calculate_rec(trienode *node, bool root, int *index)
{
    if (node && node->size > 0)
    {
        for (int i = 0; i < NUM_CHARS; i++)
        {
            if (node->children[i])
            {
                fprintf(out, "TrieNode('%c'", i);
                if (node->children[i]->terminal)
                {
                    if (node->children[i]->size > 0)
                    {
                        fprintf(out, ",%d,%d,TokenType::%s,true,true", ++(*index), node->children[i]->size, get_token_type(node->children[i]->val));
                        goto CALC;
                    }
                    else
                    {
                        fprintf(out, ",TokenType::%s", get_token_type(node->children[i]->val));
                        ++(*index);
                    }
                }
                else
                {
                    fprintf(out, ",%d,%d", ++(*index), node->children[i]->size);
                }
                if (root)
                {
                    fprintf(out, ",true");
                }
            CALC:
                fprintf(out, "), // %d\n", (*index) - 1);
                calculate_rec(node->children[i], false, index);
            }
        }
    }
}

void calculate(trienode *root)
{
    out = fopen("src/TrieStructure.inc", "w");
    if (out == NULL)
        exit(EXIT_FAILURE);
    fprintf(out, "#include<Token.hpp>\n#include<Trie.hpp>\nconst TrieNode trie[] = {\n");
    int index = 0;
    calculate_rec(root, true, &index);
    fprintf(out, "};\n");
    fclose(out);
}

int main(void)
{
    FILE *fp;
    char line[128];
    size_t len = 0;

    fp = fopen("trieinput.trie", "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);
    char c;
    int lineCount = 1;
    for (c = getc(fp); c != EOF; c = getc(fp))
        if (c == '\n')
            lineCount++;

    fseek(fp, 0, SEEK_SET);

    trienode *root = NULL;

    while (fgets(line, sizeof(line), fp) != NULL)
    {
        for (size_t i = 0; i < sizeof(line) && line[i] != '\0'; i++)
        {
            if (line[i] == '\n' || line[i] == '\r')
                line[i] = '\0';
        }
        trie_insert(&root, line);
    }
    print_trie(root);
    calculate(root);
    fclose(fp);
    exit(EXIT_SUCCESS);
}