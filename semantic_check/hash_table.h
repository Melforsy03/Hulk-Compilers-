typedef struct HashTable HashTable;

HashTable* hash_table_create();
void hash_table_free(HashTable* table);
bool hash_table_contains(HashTable* table, const char* key);
void hash_table_insert(HashTable* table, const char* key, void* value);
void* hash_table_get(HashTable* table, const char* key);
size_t hash_table_size(HashTable* table);