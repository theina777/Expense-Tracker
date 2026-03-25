#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
// These macros inject a 50ms sleep before any input, forcing the browser to repaint the frozen screen first!
#define fgets(buf, size, stream) (emscripten_sleep(50), (fgets)(buf, size, stream))
#define scanf(...) (emscripten_sleep(50), (scanf)(__VA_ARGS__))
#endif

#define MAX_EXPENSES 1000
#define FILE_NAME "expenses.dat"
#define BUDGET_FILE "budget.dat"

typedef struct {
    char date[11];      // Format: YYYY-MM-DD
    char category[50];
    float amount;
    char note[100];
} Expense;

Expense expenses[MAX_EXPENSES];
int expense_count = 0;
float monthly_budget = 0.0;

// Function Prototypes
void loadData();
void saveData();
void addExpense();
void viewTotalByMonth();
void viewCategoryTotal();
void searchExpenses();
void setBudget();
void checkBudgetWarning(const char* month);
void clearInputBuffer();
int my_strcasecmp(const char *s1, const char *s2);

int main() {
    // Disable stdout buffering so text prints immediately before the prompt
    setvbuf(stdout, NULL, _IONBF, 0);
    loadData();
#ifdef __EMSCRIPTEN__
    return 0; // Let the WebAssembly API take over, skipping the blocking menu!
#endif
    int choice;

    while (1) {
        printf("\n=== Expense & Budget Tracker ===\n");
        printf("1. Add Expense\n");
        printf("2. View Total Expense for a Month\n");
        printf("3. View Category-wise Total\n");
        printf("4. Search Expenses\n");
        printf("5. Set Monthly Budget\n");
        printf("6. Exit\n");
        printf("Enter your choice: ");
        
        if (scanf("%d", &choice) != 1) {
            printf("Invalid input! Please enter a number.\n");
            clearInputBuffer();
            continue;
        }
        clearInputBuffer();

        switch (choice) {
            case 1:
                addExpense();
                break;
            case 2:
                viewTotalByMonth();
                break;
            case 3:
                viewCategoryTotal();
                break;
            case 4:
                searchExpenses();
                break;
            case 5:
                setBudget();
                break;
            case 6:
                saveData();
                printf("Exiting program. Goodbye!\n");
                exit(0);
            default:
                printf("Invalid choice! Please select an option between 1 and 6.\n");
        }
    }
    return 0;
}

void clearInputBuffer() {
#ifndef __EMSCRIPTEN__
    int c;
    while ((c = getchar()) != '\n' && c != EOF) { }
#endif
}

int my_strcasecmp(const char *s1, const char *s2) {
    while (*s1 && *s2) {
        char c1 = (*s1 >= 'A' && *s1 <= 'Z') ? *s1 + 32 : *s1;
        char c2 = (*s2 >= 'A' && *s2 <= 'Z') ? *s2 + 32 : *s2;
        if (c1 != c2) return c1 - c2;
        s1++;
        s2++;
    }
    char c1 = (*s1 >= 'A' && *s1 <= 'Z') ? *s1 + 32 : *s1;
    char c2 = (*s2 >= 'A' && *s2 <= 'Z') ? *s2 + 32 : *s2;
    return c1 - c2;
}

void loadData() {
    FILE *file = fopen(FILE_NAME, "rb");
    if (file != NULL) {
        expense_count = fread(expenses, sizeof(Expense), MAX_EXPENSES, file);
        fclose(file);
    }
    
    FILE *b_file = fopen(BUDGET_FILE, "r");
    if (b_file != NULL) {
        if (fscanf(b_file, "%f", &monthly_budget) != 1) {
            monthly_budget = 0.0;
        }
        fclose(b_file);
    }
}

void saveData() {
    FILE *file = fopen(FILE_NAME, "wb");
    if (file != NULL) {
        fwrite(expenses, sizeof(Expense), expense_count, file);
        fclose(file);
    } else {
        printf("Error: Could not save expenses to file.\n");
    }
    
    FILE *b_file = fopen(BUDGET_FILE, "w");
    if (b_file != NULL) {
        fprintf(b_file, "%.2f", monthly_budget);
        fclose(b_file);
    } else {
        printf("Error: Could not save budget to file.\n");
    }
}

void addExpense() {
    if (expense_count >= MAX_EXPENSES) {
        printf("Error: Maximum expense limit reached.\n");
        return;
    }

    Expense e;
    printf("\nEnter Date (YYYY-MM-DD): ");
    if (fgets(e.date, sizeof(e.date), stdin) == NULL) return;
    e.date[strcspn(e.date, "\n")] = 0;
    
    // Input validation: Check length of date
    if (strlen(e.date) != 10 || e.date[4] != '-' || e.date[7] != '-') {
        printf("Invalid date format. Expected YYYY-MM-DD.\n");
        return;
    }

    printf("Enter Category (e.g., Food, Travel, Medical): ");
    if (fgets(e.category, sizeof(e.category), stdin) == NULL) return;
    e.category[strcspn(e.category, "\n")] = 0;
    
    if (strlen(e.category) == 0) {
        printf("Category cannot be empty.\n");
        return;
    }

    printf("Enter Amount: ");
    if (scanf("%f", &e.amount) != 1 || e.amount <= 0) {
        printf("Invalid amount! Must be a positive number.\n");
        clearInputBuffer();
        return;
    }
    clearInputBuffer();

    printf("Enter Note (optional): ");
    if (fgets(e.note, sizeof(e.note), stdin) == NULL) return;
    e.note[strcspn(e.note, "\n")] = 0;

    expenses[expense_count++] = e;
    printf("Expense added successfully!\n");
    
    char month[8];
    strncpy(month, e.date, 7);
    month[7] = '\0';
    checkBudgetWarning(month);
}

void viewTotalByMonth() {
    char month[8];
    printf("\nEnter Month (YYYY-MM): ");
    if (fgets(month, sizeof(month), stdin) == NULL) return;
    month[strcspn(month, "\n")] = 0;

    if (strlen(month) != 7 || month[4] != '-') {
        printf("Invalid month format. Expected YYYY-MM.\n");
        return;
    }

    float total = 0.0;
    int found = 0;
    for (int i = 0; i < expense_count; i++) {
        if (strncmp(expenses[i].date, month, 7) == 0) {
            total += expenses[i].amount;
            found = 1;
        }
    }

    if (!found) {
        printf("No expenses found for month %s.\n", month);
    } else {
        printf("\n--- Monthly Total ---");
        printf("\nTotal expenses for %s: %.2f\n", month, total);
        if (monthly_budget > 0.0) {
            printf("Monthly Budget: %.2f\n", monthly_budget);
            if (total > monthly_budget) {
                printf("\n*** WARNING: You have exceeded your monthly budget! ***\n");
            } else {
                printf("Remaining Budget: %.2f\n", monthly_budget - total);
            }
        }
    }
}

void viewCategoryTotal() {
    char category[50];
    printf("\nEnter Category: ");
    if (fgets(category, sizeof(category), stdin) == NULL) return;
    category[strcspn(category, "\n")] = 0;

    if (strlen(category) == 0) {
        printf("Category cannot be empty.\n");
        return;
    }

    float total = 0.0;
    int found = 0;
    for (int i = 0; i < expense_count; i++) {
        if (my_strcasecmp(expenses[i].category, category) == 0) { 
            total += expenses[i].amount;
            found = 1;
        }
    }

    if (!found) {
        printf("No expenses found for category '%s'.\n", category);
    } else {
        printf("\n--- Category Total ---");
        printf("\nTotal expenses for category '%s': %.2f\n", category, total);
    }
}

void searchExpenses() {
    printf("\nSearch by:\n1. Date (YYYY-MM-DD)\n2. Category\nEnter choice: ");
    int choice;
    if (scanf("%d", &choice) != 1) {
         printf("Invalid input!\n");
         clearInputBuffer();
         return;
    }
    clearInputBuffer();

    if (choice != 1 && choice != 2) {
        printf("Invalid choice. Must be 1 or 2.\n");
        return;
    }

    char query[50];
    if (choice == 1) {
        printf("Enter Date (YYYY-MM-DD): ");
    } else {
        printf("Enter Category: ");
    }
    
    if (fgets(query, sizeof(query), stdin) == NULL) return;
    query[strcspn(query, "\n")] = 0;

    int found = 0;
    printf("\n--- Search Results ---\n");
    for (int i = 0; i < expense_count; i++) {
        int match = 0;
        if (choice == 1 && strcmp(expenses[i].date, query) == 0) match = 1;
        else if (choice == 2 && my_strcasecmp(expenses[i].category, query) == 0) match = 1;

        if (match) {
            printf("Date: %s | Category: %-15s | Amount: %8.2f | Note: %s\n", 
                   expenses[i].date, expenses[i].category, expenses[i].amount, expenses[i].note);
            found = 1;
        }
    }

    if (!found) {
        printf("No matching expenses found.\n");
    }
}

void setBudget() {
    printf("\nEnter new monthly budget limit: ");
    float new_budget;
    if (scanf("%f", &new_budget) != 1 || new_budget < 0) {
        printf("Invalid amount! Budget must be a non-negative number.\n");
        clearInputBuffer();
        return;
    }
    clearInputBuffer();
    monthly_budget = new_budget;
    printf("Monthly budget successfully set to %.2f\n", monthly_budget);
}

void checkBudgetWarning(const char* month) {
    if (monthly_budget <= 0.0) return;

    float total = 0.0;
    for (int i = 0; i < expense_count; i++) {
        if (strncmp(expenses[i].date, month, 7) == 0) {
            total += expenses[i].amount;
        }
    }

    if (total > monthly_budget) {
        printf("\n*** WARNING: Monthly expense (%.2f) exceeds budget limit (%.2f) for %s! ***\n", total, monthly_budget, month);
    }
}

// ==========================================================
// WebAssembly API Definitions (For modern HTML Frontend)
// ==========================================================
#ifdef __EMSCRIPTEN__

EMSCRIPTEN_KEEPALIVE
int api_addExpense(const char* date, const char* category, float amount, const char* note) {
    if (expense_count >= MAX_EXPENSES) return 0;
    
    Expense e;
    strncpy(e.date, date, sizeof(e.date));
    e.date[sizeof(e.date)-1] = '\0';
    
    strncpy(e.category, category, sizeof(e.category));
    e.category[sizeof(e.category)-1] = '\0';
    
    e.amount = amount;
    
    strncpy(e.note, note, sizeof(e.note));
    e.note[sizeof(e.note)-1] = '\0';
    
    expenses[expense_count++] = e;
    saveData();
    return 1;
}

EMSCRIPTEN_KEEPALIVE
int api_getExpenseCount() { return expense_count; }

EMSCRIPTEN_KEEPALIVE
const char* api_getExpenseDate(int index) { return expenses[index].date; }

EMSCRIPTEN_KEEPALIVE
const char* api_getExpenseCategory(int index) { return expenses[index].category; }

EMSCRIPTEN_KEEPALIVE
float api_getExpenseAmount(int index) { return expenses[index].amount; }

EMSCRIPTEN_KEEPALIVE
const char* api_getExpenseNote(int index) { return expenses[index].note; }

EMSCRIPTEN_KEEPALIVE
float api_getBudget() { return monthly_budget; }

EMSCRIPTEN_KEEPALIVE
void api_setBudget(float budget) { 
    monthly_budget = budget; 
    saveData(); 
}

EMSCRIPTEN_KEEPALIVE
int api_deleteExpense(int index) {
    if (index < 0 || index >= expense_count) return 0;
    for (int i = index; i < expense_count - 1; i++) {
        expenses[i] = expenses[i+1];
    }
    expense_count--;
    saveData();
    return 1;
}

#endif
