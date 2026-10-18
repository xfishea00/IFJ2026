/**
 * Project: Implementace překladače imperativního jazyka IFJ26
 * File:    test_symtable.c
 * Authors: Jaroslav Baček (xbacekj00)
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "../src/symtable.h"

void test_init(void) {
    symtable_t *table = symtable_init();

    assert(table != NULL);

    printf("Test inicializace tabulky proběhl úspěšně.\n");
    symtable_free(table);
}

void test_insert_and_search(void) {
    symtable_t *table = symtable_init();
    symbol_info_t promenna1 = {TYPE_INT, SYM_VALUE, 0, NULL};
    assert (symtable_insert(table, "promenna", promenna1) == true);

    symbol_info_t *info = symtable_search(table, "promenna");
    assert (info != NULL);
    assert (info->type == TYPE_INT);
    assert (info->mode == SYM_VALUE);

    assert (symtable_search(table, "neexistujici") == NULL);

    printf("Test vkládání a vyhledávání proběhl úspěšně.\n");
    symtable_free(table);
}

void test_update(void) {
    symtable_t *table = symtable_init();

    symbol_info_t promenna1 = {TYPE_INT, SYM_VARIABLE, 0, NULL};
    symtable_insert(table, "promenna", promenna1);

    symbol_info_t promenna2 = {TYPE_STRING, SYM_VALUE, 0, NULL};
    assert(symtable_insert(table, "promenna", promenna2) == false);

    symbol_info_t *info = symtable_search(table, "promenna");
    assert (info != NULL);
    assert (info->type == TYPE_INT);
    assert (info->mode == SYM_VARIABLE);

    printf("Test aktualizace proběhl úspěšně.\n");
    symtable_free(table);
}

void test_collisions(void) {
    symtable_t *table = symtable_init();

    symbol_info_t promenna1 = {TYPE_INT, SYM_VARIABLE, 0, NULL};
    assert(symtable_insert(table, "promenna1", promenna1) == true);
    assert(symtable_insert(table, "promenna2", promenna1) == true);
    assert(symtable_insert(table, "promenna3", promenna1) == true);
    assert(symtable_insert(table, "promenna4", promenna1) == true);

    assert(symtable_search(table, "promenna1") != NULL);
    assert(symtable_search(table, "promenna2") != NULL);
    assert(symtable_search(table, "promenna3") != NULL);
    assert(symtable_search(table, "promenna4") != NULL);

    printf("Test kolizí proběhl úspěšně.\n");
    symtable_free(table);
}

void test_delete(void) {
    symtable_t *table = symtable_init();
    symbol_info_t promenna1 = {TYPE_INT, SYM_VARIABLE, 0, NULL};

    symtable_insert(table, "promenna", promenna1);
    symtable_insert(table, "promenna2", promenna1);
    symtable_insert(table, "promenna3", promenna1);

    assert(symtable_delete(table, "promenna2") == true);
    assert(symtable_search(table, "promenna2") == NULL);

    assert(symtable_search(table, "promenna") != NULL);
    assert(symtable_search(table, "promenna3") != NULL);

    printf("Test mazání proběhl úspěšně.\n");
    symtable_free(table);
}

int main(void) {
    printf("Spouštím testy pro symtable...\n");
    printf("-------------------------------\n");

    printf("Test inicializace tabulky...\n");
    test_init();

    printf("Test vkládání a vyhledávání...\n");
    test_insert_and_search();

    printf("Test aktualizace...\n");
    test_update();

    printf("Test kolizí...\n");
    test_collisions();

    printf("Test mazání...\n");
    test_delete();

    printf("-------------------------------\n");
    printf("Všechny testy proběhly úspěšně!\n");
    return 0;
}
