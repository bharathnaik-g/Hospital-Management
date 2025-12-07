// triage.c
// Compile: gcc triage.c -o triage
// Usage:
//   ./triage list
//   ./triage add 101 John Doe 30 9
//   ./triage update 101 7
//   ./triage delete 101

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define STORAGE_FILE "patients.json"

typedef struct Patient
{
    int patientID;
    char name[256];
    int age;
    int severity;
    int height;
    struct Patient *left, *right;
} Patient;

// ---------- AVL utilities ----------
int max(int a, int b) { return (a > b) ? a : b; }
int getHeight(Patient *n) { return n ? n->height : 0; }
int getBalanceFactor(Patient *n) { return n ? getHeight(n->left) - getHeight(n->right) : 0; }

Patient *rightRotate(Patient *y)
{
    Patient *x = y->left;
    Patient *T2 = x->right;
    x->right = y;
    y->left = T2;
    y->height = 1 + max(getHeight(y->left), getHeight(y->right));
    x->height = 1 + max(getHeight(x->left), getHeight(x->right));
    return x;
}

Patient *leftRotate(Patient *x)
{
    Patient *y = x->right;
    Patient *T2 = y->left;
    y->left = x;
    x->right = T2;
    x->height = 1 + max(getHeight(x->left), getHeight(x->right));
    y->height = 1 + max(getHeight(y->left), getHeight(y->right));
    return y;
}

Patient *createPatientNode(int id, const char *name, int age, int severity)
{
    Patient *p = (Patient *)malloc(sizeof(Patient));
    p->patientID = id;
    strncpy(p->name, name, sizeof(p->name) - 1);
    p->name[sizeof(p->name) - 1] = '\0';
    p->age = age;
    p->severity = severity;
    p->height = 1;
    p->left = p->right = NULL;
    return p;
}

Patient *insertPatient(Patient *node, int id, const char *name, int age, int severity)
{
    if (!node)
        return createPatientNode(id, name, age, severity);

    if (severity < node->severity)
        node->left = insertPatient(node->left, id, name, age, severity);
    else
        node->right = insertPatient(node->right, id, name, age, severity);

    node->height = 1 + max(getHeight(node->left), getHeight(node->right));
    int balance = getBalanceFactor(node);

    if (balance > 1 && severity < node->left->severity)
        return rightRotate(node);
    if (balance < -1 && severity > node->right->severity)
        return leftRotate(node);
    if (balance > 1 && severity > node->left->severity)
    {
        node->left = leftRotate(node->left);
        return rightRotate(node);
    }
    if (balance < -1 && severity < node->right->severity)
    {
        node->right = rightRotate(node->right);
        return leftRotate(node);
    }
    return node;
}

Patient *minValueNode(Patient *n)
{
    Patient *cur = n;
    while (cur && cur->left)
        cur = cur->left;
    return cur;
}

// Search, delete, update functions
Patient *searchByID(Patient *root, int id)
{
    if (!root) return NULL;
    if (root->patientID == id) return root;
    Patient *l = searchByID(root->left, id);
    if (l) return l;
    return searchByID(root->right, id);
}

Patient *deleteByID(Patient *root, int id)
{
    if (!root) return NULL;

    if (root->patientID == id)
    {
        if (!root->left || !root->right)
        {
            Patient *temp = root->left ? root->left : root->right;
            if (!temp)
            {
                free(root);
                return NULL;
            }
            else
            {
                Patient *copy = createPatientNode(temp->patientID, temp->name, temp->age, temp->severity);
                copy->left = temp->left;
                copy->right = temp->right;
                copy->height = temp->height;
                free(temp);
                free(root);
                return copy;
            }
        }
        else
        {
            Patient *succ = minValueNode(root->right);
            root->patientID = succ->patientID;
            strncpy(root->name, succ->name, sizeof(root->name) - 1);
            root->name[sizeof(root->name) - 1] = '\0';
            root->age = succ->age;
            root->severity = succ->severity;
            root->right = deleteByID(root->right, succ->patientID);
        }
    }
    else
    {
        root->left = deleteByID(root->left, id);
        root->right = deleteByID(root->right, id);
    }

    if (!root) return root;

    root->height = 1 + max(getHeight(root->left), getHeight(root->right));
    int balance = getBalanceFactor(root);

    if (balance > 1 && getBalanceFactor(root->left) >= 0)
        return rightRotate(root);
    if (balance > 1 && getBalanceFactor(root->left) < 0)
    {
        root->left = leftRotate(root->left);
        return rightRotate(root);
    }
    if (balance < -1 && getBalanceFactor(root->right) <= 0)
        return leftRotate(root);
    if (balance < -1 && getBalanceFactor(root->right) > 0)
    {
        root->right = rightRotate(root->right);
        return leftRotate(root);
    }
    return root;
}

Patient *updateSeverityByID(Patient *root, int id, int newSeverity)
{
    Patient *p = searchByID(root, id);
    if (!p) return root;

    char namebuf[256];
    strncpy(namebuf, p->name, sizeof(namebuf) - 1);
    namebuf[sizeof(namebuf) - 1] = '\0';
    int age = p->age;

    root = deleteByID(root, id);
    root = insertPatient(root, id, namebuf, age, newSeverity);
    return root;
}

// Reverse inorder traversal
void reverseInorderCollect(Patient *root, Patient ***arr_ptr, int *size, int *cap)
{
    if (!root) return;
    reverseInorderCollect(root->right, arr_ptr, size, cap);
    if (*size + 1 > *cap)
    {
        *cap = (*cap == 0) ? 8 : (*cap * 2);
        *arr_ptr = (Patient **)realloc(*arr_ptr, (*cap) * sizeof(Patient *));
    }
    (*arr_ptr)[(*size)++] = root;
    reverseInorderCollect(root->left, arr_ptr, size, cap);
}

// JSON persistence
void jsonEscapeString(const char *in, char *out, int outlen)
{
    int j = 0;
    for (int i = 0; in[i] && j < outlen - 1; ++i)
    {
        char c = in[i];
        if (c == '\"' && j + 2 < outlen - 1) { out[j++] = '\\'; out[j++] = '\"'; }
        else if (c == '\\' && j + 2 < outlen - 1) { out[j++] = '\\'; out[j++] = '\\'; }
        else if (c == '\n' && j + 2 < outlen - 1) { out[j++] = '\\'; out[j++] = 'n'; }
        else out[j++] = c;
    }
    out[j] = '\0';
}

int saveToFile(Patient *root)
{
    Patient **arr = NULL;
    int size = 0, cap = 0;
    reverseInorderCollect(root, &arr, &size, &cap);

    FILE *f = fopen(STORAGE_FILE, "w");
    if (!f) return 0;
    fprintf(f, "[\n");
    for (int i = 0; i < size; ++i)
    {
        char esc[1024];
        jsonEscapeString(arr[i]->name, esc, sizeof(esc));
        fprintf(f, "  {\"id\":%d,\"name\":\"%s\",\"age\":%d,\"severity\":%d}%s\n",
                arr[i]->patientID, esc, arr[i]->age, arr[i]->severity, (i == size - 1) ? "" : ",");
    }
    fprintf(f, "]\n");
    fclose(f);
    if (arr) free(arr);
    return 1;
}

int loadFromFile(Patient **root_ptr)
{
    FILE *f = fopen(STORAGE_FILE, "r");
    if (!f) return 0;
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = (char *)malloc(len + 1);
    fread(buf, 1, len, f);
    buf[len] = 0;
    fclose(f);

    char *p = buf;
    while ((p = strchr(p, '{')) != NULL)
    {
        char *obj = p;
        char *idPos = strstr(obj, "\"id\":");
        if (!idPos) { p++; continue; }
        int id = 0; sscanf(idPos + 5, "%d", &id);

        char *namePos = strstr(obj, "\"name\":");
        char nameBuf[256] = "";
        if (namePos)
        {
            char *q = strchr(namePos, '\"'); 
            if (q) { q = strchr(q + 1, '\"'); 
                if (q) { 
                    char *start = q + 1; 
                    char *end = strchr(start, '\"'); 
                    if (end && end - start < (int)sizeof(nameBuf))
                    {
                        int n = end - start;
                        strncpy(nameBuf, start, n);
                        nameBuf[n] = '\0';
                    } else {
                        int k=0; char *t=start;
                        while (t && *t && *t != '\"' && k < (int)sizeof(nameBuf)-1) nameBuf[k++] = *t++;
                        nameBuf[k] = 0;
                    }
                }
            }
        }

        char *agePos = strstr(obj, "\"age\":"); int age=0;
        if(agePos) sscanf(agePos+6, "%d", &age);

        char *sevPos = strstr(obj, "\"severity\":"); int sev=0;
        if(sevPos) sscanf(sevPos+11, "%d", &sev);

        *root_ptr = insertPatient(*root_ptr, id, nameBuf, age, sev);

        p = obj + 1;
    }

    free(buf);
    return 1;
}

// JSON output
void outputListJSON(Patient *root)
{
    Patient **arr = NULL; int size=0, cap=0;
    reverseInorderCollect(root, &arr, &size, &cap);
    printf("[");
    for(int i=0;i<size;i++)
    {
        char esc[1024]; jsonEscapeString(arr[i]->name, esc, sizeof(esc));
        printf("%s{\"id\":%d,\"name\":\"%s\",\"age\":%d,\"severity\":%d}",
               (i==0)?"":",", arr[i]->patientID, esc, arr[i]->age, arr[i]->severity);
    }
    printf("]\n");
    if(arr) free(arr);
}

// CLI usage
void printUsage(const char *prog)
{
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  %s list\n", prog);
    fprintf(stderr, "  %s add <id> <name with spaces> <age> <severity>\n", prog);
    fprintf(stderr, "  %s update <id> <newSeverity>\n", prog);
    fprintf(stderr, "  %s delete <id>\n", prog);
}

int main(int argc, char **argv)
{
    Patient *root = NULL;
    loadFromFile(&root);

    if(argc<2) { printUsage(argv[0]); return 1; }

    const char *cmd = argv[1];

    if(strcmp(cmd,"list")==0)
    {
        outputListJSON(root);
        return 0;
    }
    else if(strcmp(cmd,"add")==0)
    {
        if(argc<5) { fprintf(stderr,"add requires at least 4 arguments: id name age severity\n"); return 1; }

        int id = atoi(argv[2]);

        // Combine all args except last two as name
        char nameBuf[256]="";
        for(int i=3;i<argc-2;i++)
        {
            strncat(nameBuf, argv[i], sizeof(nameBuf)-strlen(nameBuf)-1);
            if(i<argc-3) strncat(nameBuf," ", sizeof(nameBuf)-strlen(nameBuf)-1);
        }

        int age = atoi(argv[argc-2]);
        int severity = atoi(argv[argc-1]);

        root = insertPatient(root, id, nameBuf, age, severity);
        saveToFile(root);
        printf("{\"ok\":true}\n");
        return 0;
    }
    else if(strcmp(cmd,"update")==0)
    {
        if(argc<4) { fprintf(stderr,"update requires id and newSeverity\n"); return 1; }
        int id = atoi(argv[2]);
        int newS = atoi(argv[3]);
        root = updateSeverityByID(root, id, newS);
        saveToFile(root);
        printf("{\"ok\":true}\n");
        return 0;
    }
    else if(strcmp(cmd,"delete")==0)
    {
        if(argc<3) { fprintf(stderr,"delete requires id\n"); return 1; }
        int id = atoi(argv[2]);
        root = deleteByID(root, id);
        saveToFile(root);
        printf("{\"ok\":true}\n");
        return 0;
    }
    else
    {
        printUsage(argv[0]);
        return 1;
    }

    return 0;
}
