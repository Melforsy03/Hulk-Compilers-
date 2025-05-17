typedef struct ListNode {
    void* data;
    struct ListNode* next;
} ListNode;

typedef struct {
    ListNode* head;
    ListNode* tail;
    int count;
} List;

List* list_create();
void list_append(List* list, void* data);
void* list_get(List* list, int index);
void list_remove(List* list, void* data, void (*free_data)(void*));
int list_length(List* list);
void list_free(List* list, void (*free_data)(void*));
