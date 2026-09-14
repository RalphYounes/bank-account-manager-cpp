# Bank Account Manager (C++)

A console-based bank account management application developed for a Data Structures course.

## Features

- Manage users and bank accounts
- Store users, accounts, and transactions using linked lists
- Transfer money between accounts
- Support USD, EUR, and Lebanese Lira currency conversion
- Enforce daily deposit and monthly withdrawal limits
- Sort transactions by date
- Delete transactions before a chosen date
- Read and save data using a text file

## Data Structures Used

- Doubly linked list for users
- Singly linked lists for accounts and transactions
- Hash maps for currency exchange rates
- Hash sets to avoid duplicate IBANs when saving data

## Files

- `project.cpp`: application source code
- `User.txt`: fictional sample users, accounts, and transactions

## How to Run

1. Open the project in Visual Studio or another C++ compiler.
2. Make sure `User.txt` is in the same folder as the program's executable.
3. Build and run the application.
4. Use the menu to view accounts, transfer funds, create users and accounts, sort transactions, or remove old transactions.

## Notes

All data in `User.txt` is fictional and included only as sample data for demonstration purposes.
