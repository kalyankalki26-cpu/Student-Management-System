/*
 * Student Management System
 * -------------------------
 * A simple console application written in C that stores student records
 * in a plain-text file. Supports adding, viewing, searching, updating and
 * deleting students, automatic grade calculation and class statistics.
 *
 * Record format (one per line):  regd|name|marks
 */

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DATA_FILE   "students.txt"
#define TEMP_FILE   "students.tmp"
#define NAME_LEN    50
#define LINE_LEN    256
#define PASS_MARKS  50.0f

typedef struct
{
    int   regd;
    char  name[NAME_LEN];
    float marks;
} Student;

/* ------------------------------------------------------------------ */
/* Input helpers                                                       */
/* ------------------------------------------------------------------ */

static void trim(char *s)
{
    size_t len = strlen(s);
    char *p = s;

    while (len > 0 && isspace((unsigned char)s[len - 1]))
        s[--len] = '\0';

    while (*p && isspace((unsigned char)*p))
        p++;

    if (p != s)
        memmove(s, p, strlen(p) + 1);
}

/* Reads one line safely. Returns 0 on EOF. */
static int readLine(const char *prompt, char *buf, size_t size)
{
    if (prompt)
        printf("%s", prompt);

    if (fgets(buf, (int)size, stdin) == NULL)
        return 0;

    if (strchr(buf, '\n') == NULL)          /* discard overly long input */
    {
        int c;
        while ((c = getchar()) != '\n' && c != EOF)
            ;
    }

    buf[strcspn(buf, "\r\n")] = '\0';
    return 1;
}

static int readInt(const char *prompt, int *out)
{
    char buf[LINE_LEN];
    char *end;

    while (readLine(prompt, buf, sizeof buf))
    {
        errno = 0;
        long v = strtol(buf, &end, 10);

        while (*end && isspace((unsigned char)*end))
            end++;

        if (end != buf && *end == '\0' && errno == 0 &&
            v >= INT_MIN && v <= INT_MAX)
        {
            *out = (int)v;
            return 1;
        }
        printf("Invalid input! Please enter a whole number.\n");
    }
    return 0;
}

static int readMarks(float *out)
{
    char buf[LINE_LEN];
    char *end;

    while (readLine("Enter Marks (0-100): ", buf, sizeof buf))
    {
        double v = strtod(buf, &end);

        while (*end && isspace((unsigned char)*end))
            end++;

        if (end != buf && *end == '\0' && v >= 0.0 && v <= 100.0)
        {
            *out = (float)v;
            return 1;
        }
        printf("Invalid marks! Please enter marks between 0 and 100.\n");
    }
    return 0;
}

static int readName(const char *prompt, char *name)
{
    char buf[LINE_LEN];

    while (readLine(prompt, buf, sizeof buf))
    {
        trim(buf);

        if (buf[0] == '\0')
            printf("Name cannot be empty.\n");
        else if (strlen(buf) >= NAME_LEN)
            printf("Name is too long (max %d characters).\n", NAME_LEN - 1);
        else if (strchr(buf, '|') != NULL)
            printf("Name cannot contain the '|' character.\n");
        else
        {
            strcpy(name, buf);
            return 1;
        }
    }
    return 0;
}

static int confirm(const char *prompt)
{
    char buf[LINE_LEN];

    if (!readLine(prompt, buf, sizeof buf))
        return 0;

    return buf[0] == 'y' || buf[0] == 'Y';
}

/* ------------------------------------------------------------------ */
/* Record helpers                                                      */
/* ------------------------------------------------------------------ */

static char getGrade(float marks)
{
    if (marks >= 90) return 'A';
    if (marks >= 80) return 'B';
    if (marks >= 70) return 'C';
    if (marks >= 60) return 'D';
    if (marks >= 50) return 'E';
    return 'F';
}

/* Accepts the current "regd|name|marks" format and the old
 * "regd name marks" format (single-word names) for compatibility. */
static int parseRecord(const char *line, Student *s)
{
    if (sscanf(line, "%d|%49[^|]|%f", &s->regd, s->name, &s->marks) == 3)
        return 1;

    if (sscanf(line, "%d %49s %f", &s->regd, s->name, &s->marks) == 3)
        return 1;

    return 0;
}

static void writeRecord(FILE *fp, const Student *s)
{
    fprintf(fp, "%d|%s|%.2f\n", s->regd, s->name, s->marks);
}

/* Returns 1 and fills *out if the student exists. */
static int findStudent(int regd, Student *out)
{
    FILE *fp = fopen(DATA_FILE, "r");
    char line[LINE_LEN];
    Student s;

    if (fp == NULL)
        return 0;

    while (fgets(line, sizeof line, fp))
    {
        if (parseRecord(line, &s) && s.regd == regd)
        {
            if (out)
                *out = s;
            fclose(fp);
            return 1;
        }
    }

    fclose(fp);
    return 0;
}

static void printStudent(const Student *s)
{
    printf("Registration No : %d\n", s->regd);
    printf("Name            : %s\n", s->name);
    printf("Marks           : %.2f\n", s->marks);
    printf("Grade           : %c\n", getGrade(s->marks));
}

/*
 * Rewrites the data file, deleting the record with the given number
 * (replacement == NULL) or replacing it.
 * Returns 1 on success, 0 if not found, -1 on file error.
 */
static int modifyRecord(int target, const Student *replacement)
{
    FILE *in = fopen(DATA_FILE, "r");
    FILE *out;
    char line[LINE_LEN];
    Student s;
    int found = 0;

    if (in == NULL)
        return 0;

    out = fopen(TEMP_FILE, "w");
    if (out == NULL)
    {
        fclose(in);
        return -1;
    }

    while (fgets(line, sizeof line, in))
    {
        if (!parseRecord(line, &s))
            continue;

        if (s.regd == target)
        {
            found = 1;
            if (replacement)
                writeRecord(out, replacement);
        }
        else
        {
            writeRecord(out, &s);
        }
    }

    fclose(in);

    if (fclose(out) != 0)
    {
        remove(TEMP_FILE);
        return -1;
    }

    if (!found)
    {
        remove(TEMP_FILE);
        return 0;
    }

    if (remove(DATA_FILE) != 0 || rename(TEMP_FILE, DATA_FILE) != 0)
        return -1;

    return 1;
}

/* ------------------------------------------------------------------ */
/* Menu actions                                                        */
/* ------------------------------------------------------------------ */

static void addStudent(void)
{
    Student s;
    FILE *fp;

    if (!readInt("\nEnter Registration Number: ", &s.regd))
        return;

    if (s.regd <= 0)
    {
        printf("\nRegistration number must be a positive number!\n");
        return;
    }

    if (findStudent(s.regd, NULL))
    {
        printf("\nRegistration number already exists!\n");
        return;
    }

    if (!readName("Enter Name: ", s.name) || !readMarks(&s.marks))
        return;

    fp = fopen(DATA_FILE, "a");
    if (fp == NULL)
    {
        printf("\nFile cannot be opened!\n");
        return;
    }

    writeRecord(fp, &s);
    fclose(fp);

    printf("\n--- Student Added Successfully! ---\n");
}

static void viewStudents(void)
{
    FILE *fp = fopen(DATA_FILE, "r");
    char line[LINE_LEN];
    Student s;
    int count = 0;

    if (fp == NULL)
    {
        printf("\nNo student records found!\n");
        return;
    }

    printf("\n===== STUDENT DETAILS =====\n");
    printf("%-8s %-30s %-8s %-5s\n", "Reg No", "Name", "Marks", "Grade");
    printf("-------------------------------------------------------\n");

    while (fgets(line, sizeof line, fp))
    {
        if (parseRecord(line, &s))
        {
            printf("%-8d %-30s %-8.2f %-5c\n",
                   s.regd, s.name, s.marks, getGrade(s.marks));
            count++;
        }
    }

    fclose(fp);

    if (count == 0)
        printf("No student records found!\n");
    else
        printf("-------------------------------------------------------\n"
               "Total students: %d\n", count);
}

static void searchStudent(void)
{
    int regd;
    Student s;

    if (!readInt("\nEnter Registration Number to Search: ", &regd))
        return;

    if (findStudent(regd, &s))
    {
        printf("\n--- Student Found ---\n");
        printStudent(&s);
    }
    else
    {
        printf("\nStudent Not Found!\n");
    }
}

static void deleteStudent(void)
{
    int regd;
    Student s;

    if (!readInt("\nEnter Registration Number to Delete: ", &regd))
        return;

    if (!findStudent(regd, &s))
    {
        printf("\nStudent Not Found!\n");
        return;
    }

    printf("\n");
    printStudent(&s);

    if (!confirm("\nAre you sure you want to delete this record? (y/n): "))
    {
        printf("\nDeletion cancelled.\n");
        return;
    }

    if (modifyRecord(regd, NULL) == 1)
        printf("\nStudent deleted successfully!\n");
    else
        printf("\nError: could not update the data file!\n");
}

static void updateStudent(void)
{
    int regd;
    Student s;

    if (!readInt("\nEnter Registration Number to Update: ", &regd))
        return;

    if (!findStudent(regd, &s))
    {
        printf("\nStudent Not Found!\n");
        return;
    }

    printf("\nCurrent details:\n");
    printStudent(&s);
    printf("\n");

    if (!readName("Enter New Name: ", s.name) || !readMarks(&s.marks))
        return;

    if (modifyRecord(regd, &s) == 1)
        printf("\nStudent updated successfully!\n");
    else
        printf("\nError: could not update the data file!\n");
}

static void showStatistics(void)
{
    FILE *fp = fopen(DATA_FILE, "r");
    char line[LINE_LEN];
    Student s, top, low;
    int count = 0, passed = 0;
    double total = 0.0;

    if (fp == NULL)
    {
        printf("\nNo student records found!\n");
        return;
    }

    while (fgets(line, sizeof line, fp))
    {
        if (!parseRecord(line, &s))
            continue;

        if (count == 0 || s.marks > top.marks) top = s;
        if (count == 0 || s.marks < low.marks) low = s;

        total += s.marks;
        if (s.marks >= PASS_MARKS)
            passed++;
        count++;
    }

    fclose(fp);

    if (count == 0)
    {
        printf("\nNo student records found!\n");
        return;
    }

    printf("\n===== CLASS STATISTICS =====\n");
    printf("Total Students : %d\n", count);
    printf("Average Marks  : %.2f\n", total / count);
    printf("Highest Marks  : %.2f (%s)\n", top.marks, top.name);
    printf("Lowest Marks   : %.2f (%s)\n", low.marks, low.name);
    printf("Passed         : %d (%.1f%%)\n", passed, 100.0 * passed / count);
    printf("Failed         : %d\n", count - passed);
}

/* ------------------------------------------------------------------ */
/* Main                                                                */
/* ------------------------------------------------------------------ */

int main(void)
{
    int choice = 0;

    do
    {
        printf("\n\n===== STUDENT MANAGEMENT SYSTEM =====\n");
        printf("1. Add Student\n");
        printf("2. View Students\n");
        printf("3. Search Student\n");
        printf("4. Delete Student\n");
        printf("5. Update Student\n");
        printf("6. Class Statistics\n");
        printf("7. Exit\n");

        if (!readInt("\nEnter your choice: ", &choice))
            break;                                  /* EOF: quit cleanly */

        switch (choice)
        {
            case 1: addStudent();     break;
            case 2: viewStudents();   break;
            case 3: searchStudent();  break;
            case 4: deleteStudent();  break;
            case 5: updateStudent();  break;
            case 6: showStatistics(); break;
            case 7: printf("\nThank You!\n"); break;
            default: printf("\n--- Invalid Choice! ---\n");
        }
    } while (choice != 7);

    return 0;
}
