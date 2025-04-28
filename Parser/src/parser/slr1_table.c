#include "slr1_table.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Función auxiliar para recolectar todos los estados del autómata
void collect_states_slr(State* start, State** states, int* state_count) 
{
    int visited_capacity = 1000;
    int visited_count = 0;
    states[visited_count++] = start;

    for (int i = 0; i < visited_count; ++i) 
    {
        State* current = states[i];
        Transition* t = current->transitions;
        while (t) 
        {
            int found = 0;
            for (int j = 0; j < visited_count; ++j) 
            {
                if (states[j] == t->next_state) 
                {
                    found = 1;
                    break;
                }
            }
            if (!found) 
            {
                states[visited_count++] = t->next_state;
            }
            t = t->next;
        }
    }

    *state_count = visited_count;
}

int index_of_symbol_slr(Symbol** list, int count, Symbol* s) 
{
    for (int i = 0; i < count; ++i) 
    {
        if (strcmp(list[i]->name, s->name) == 0) 
        {
            return i;
        }
    }
    return -1;
}

SLR1Table* build_slr1_table(State* start, Grammar* grammar, ContainerSet** follows) 
{
    SLR1Table* table = (SLR1Table*)malloc(sizeof(SLR1Table));

    State* states[1000];
    int state_count = 0;
    collect_states_slr(start, states, &state_count);

    table->state_count = state_count;
    table->terminal_count = grammar->terminals_count;
    table->nonterminal_count = grammar->nonterminals_count;
    table->grammar = grammar;

    table->action = (ActionEntrySLR**)malloc(sizeof(ActionEntrySLR*) * state_count);
    for (int i = 0; i < state_count; ++i) 
    {
        table->action[i] = (ActionEntrySLR*)malloc(sizeof(ActionEntrySLR) * grammar->terminals_count);
        for (int j = 0; j < grammar->terminals_count; ++j) 
        {
            table->action[i][j].action = ACTION_ERROR;
            table->action[i][j].value = -1;
        }
    }

    table->goto_table = (int**)malloc(sizeof(int*) * state_count);
    for (int i = 0; i < state_count; ++i) 
    {
        table->goto_table[i] = (int*)malloc(sizeof(int) * grammar->nonterminals_count);
        for (int j = 0; j < grammar->nonterminals_count; ++j) 
        {
            table->goto_table[i][j] = -1;
        }
    }

    for (int i = 0; i < state_count; ++i) 
    {
        State* current = states[i];

        // REDUCE o ACCEPT
        for (int j = 0; j < current->item_count; ++j) 
        {
            Item* item = current->items[j];

            if (is_reduce_item(item)) 
            {
                if (strcmp(item->production->left->name, grammar->startSymbol->name) == 0) 
                {
                    // ACCEPT
                    int dollar_idx = index_of_symbol_slr(grammar->terminals, grammar->terminals_count, grammar->eof);
                    if (dollar_idx >= 0) 
                    {
                        table->action[i][dollar_idx].action = ACTION_ACCEPT;
                    }
                } 
                else 
                {
                    // REDUCE: SOLO en símbolos del FOLLOW(X)
                    int left_idx = -1;
                    for (int k = 0; k < grammar->nonterminals_count; ++k) 
                    {
                        if (strcmp(grammar->nonterminals[k]->name, item->production->left->name) == 0) 
                        {
                            left_idx = k;
                            break;
                        }
                    }

                    if (left_idx >= 0) 
                    {
                        ContainerSet* follow_set = follows[left_idx];

                        for (int t = 0; t < grammar->terminals_count; ++t) 
                        {
                            if (set_contains_symbol(follow_set, grammar->terminals[t])) 
                            {
                                if (table->action[i][t].action == ACTION_ERROR) 
                                {
                                    table->action[i][t].action = ACTION_REDUCE;
                                    table->action[i][t].value = item->production->number;
                                }
                            }
                        }
                    }
                }
            }
        }

        // SHIFT y GOTO
        Transition* t = current->transitions;
        while (t) {
            int next_idx = -1;
            for (int k = 0; k < state_count; ++k) 
            {
                if (states[k] == t->next_state) 
                {
                    next_idx = k;
                    break;
                }
            }

            if (t->symbol->type == TERMINAL) 
            {
                int symbol_idx = index_of_symbol_slr(grammar->terminals, grammar->terminals_count, t->symbol);
                if (symbol_idx >= 0) 
                {
                    table->action[i][symbol_idx].action = ACTION_SHIFT;
                    table->action[i][symbol_idx].value = next_idx;
                }
            } 
            else if (t->symbol->type == NON_TERMINAL) 
            {
                int symbol_idx = index_of_symbol_slr(grammar->nonterminals, grammar->nonterminals_count, t->symbol);
                if (symbol_idx >= 0) 
                {
                    table->goto_table[i][symbol_idx] = next_idx;
                }
            }

            t = t->next;
        }
    }

    return table;
}

void print_slr1_table(SLR1Table* table) 
{
    printf("\nSLR(1) ACTION Table:\n");
    for (int i = 0; i < table->state_count; ++i) 
    {
        printf("State %d:\n", i);
        for (int j = 0; j < table->terminal_count; ++j) 
        {
            if (table->action[i][j].action == ACTION_SHIFT)
                printf("  shift %d with '%s'\n", table->action[i][j].value, table->grammar->terminals[j]->name);
            else if (table->action[i][j].action == ACTION_REDUCE)
                printf("  reduce %d with '%s'\n", table->action[i][j].value, table->grammar->terminals[j]->name);
            else if (table->action[i][j].action == ACTION_ACCEPT)
                printf("  accept with '%s'\n", table->grammar->terminals[j]->name);
        }
    }

    printf("\nSLR(1) GOTO Table:\n");
    for (int i = 0; i < table->state_count; ++i) 
    {
        printf("State %d:\n", i);
        for (int j = 0; j < table->nonterminal_count; ++j) 
        {
            if (table->goto_table[i][j] != -1)
                printf("  goto %d with '%s'\n", table->goto_table[i][j], table->grammar->nonterminals[j]->name);
        }
    }
}

void free_slr1_table(SLR1Table* table) 
{
    if (!table) return;

    for (int i = 0; i < table->state_count; ++i) 
    {
        free(table->action[i]);
        free(table->goto_table[i]);
    }
    free(table->action);
    free(table->goto_table);
    free(table);
}