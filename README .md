# Student Management System (C)

A console-based student record manager written in C. Records are stored in a
plain text file, so no database or external library is needed.

## Features

- Add students with duplicate registration-number checking
- View all students in a formatted table with automatic grades
- Search a student by registration number
- Update and delete records (with delete confirmation)
- Class statistics: average, highest, lowest, pass/fail count
- Input validation (numbers, marks range 0-100, names with spaces)

## Grading Scale

| Marks   | Grade |
|---------|-------|
| 90-100  | A     |
| 80-89   | B     |
| 70-79   | C     |
| 60-69   | D     |
| 50-59   | E     |
| below 50| F     |

## Build and Run

Requires a C compiler (GCC or Clang).

```bash
make
./student_management
```

Or compile manually:

```bash
gcc -Wall -Wextra -std=c99 -o student_management main.c
```

On Windows with MinGW: `gcc main.c -o student_management.exe`

## Data Format

Records are saved in `students.txt`, one per line:

```
101|Priya Sharma|87.50
```

Files from the older space-separated format (`101 Priya 87.50`) are still readable.

## Project Structure

```
.
├── main.c
├── Makefile
├── README.md
├── LICENSE
└── .gitignore
```

## License

MIT. See [LICENSE](LICENSE).
