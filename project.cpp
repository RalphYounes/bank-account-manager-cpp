#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <windows.h>
#include <unordered_map>
#include <stdexcept>
#include <ctime>
#include <iomanip>
#include <unordered_set>
using namespace std;


unordered_map<string, double> exchangeRates =
{
    {"$_€", 0.92},  // 1 USD to EUR
    {"€_$", 1.09},  // 1 EUR to USD
    {"$_L.L", 1500},  // 1 USD to LBP
    {"L.L_$", 0.00065},  // 1 LBP to USD
    {"€_L.L", 1600},  // 1 EUR to LBP
    {"L.L_€", 0.00068}   // 1 LBP to EUR
};

struct transaction
{
    string date;
    double amount;
    transaction* next;
};

struct account
{
    string IBAN;
    string accountName;
    double balance;
    string currency;
    double limitDepositPerDay;
    double limitWithdrawPerMonth;
    transaction* txn;
    account* next;
};

struct user
{
    int userID;
    string fname;
    string lname;
    account* acct;
    user* next, * previous;
};

struct userList
{
    user* head, * tail;
};

void InitializeUsers(userList& list)
{
    list.head = NULL;
    list.tail = NULL;
}

void addUser(userList& list, int userID, const string& fname, const string& lname)
{
    user* newUser = new user{ userID, fname, lname, NULL, NULL, NULL };

    if (list.head == NULL)
    {
        list.head = list.tail = newUser;
    }
    else
    {
        list.tail->next = newUser;
        newUser->previous = list.tail;
        list.tail = newUser;
    }
}

void addAccount(user* currentUser, const string& IBAN, const string& accountName, double balance,
    const string& currency, double limitDepositPerDay, double limitWithdrawPerMonth)
{

    account* newAccount = new account{ IBAN, accountName, balance, currency, limitDepositPerDay, limitWithdrawPerMonth, NULL, NULL };

    if (currentUser->acct == NULL)
    {
        currentUser->acct = newAccount;
    }
    else
    {
        account* lastAccount = currentUser->acct;
        while (lastAccount->next != NULL)
        {
            lastAccount = lastAccount->next;
        }
        lastAccount->next = newAccount;
    }
}

void addTransaction(account* currentAccount, const string& date, double amount)
{
    transaction* newTransaction = new transaction{ date, amount, NULL };
    if (currentAccount->txn == NULL)
    {
        currentAccount->txn = newTransaction;
    }
    else
    {
        transaction* lastTxn = currentAccount->txn;
        while (lastTxn->next != NULL)
        {
            lastTxn = lastTxn->next;
        }
        lastTxn->next = newTransaction;
    }
}


double convertCurrency(double amount, const string& fromCurrency, const string& toCurrency)
{
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);

    string key;
    if (fromCurrency == "$" && toCurrency != "L.L")
        key = "$_€";
    if (fromCurrency == "$" && toCurrency == "L.L")
        key = "$_L.L";
    if (fromCurrency != "L.L" && toCurrency == "$")
        key = "€_$";
    if (fromCurrency != "$" && toCurrency == "L.L")
        key = "€_L.L";
    if (fromCurrency == "L.L" && toCurrency != "$")
        key = "L.L_€";
    if (fromCurrency == "L.L" && toCurrency == "$")
        key = "L.L_$";

    if (exchangeRates.find(key) == exchangeRates.end())
    {
        throw invalid_argument("Exchange rate not available");
    }
    return amount * exchangeRates[key];
}


tm parseDate(const string& date)
{
    tm parsedDate = {};
    istringstream ss(date);
    ss >> get_time(&parsedDate, "%d/%m/%Y");
    if (ss.fail())
    {
        throw invalid_argument("Invalid date format: " + date);
    }
    return parsedDate;
}



bool transferAmount(userList& user_list, double amount, const string& sourceIBAN, const string& destinationIBAN)
{
    user* currentUser = user_list.head;
    account* sourceAccount = NULL;
    account* destinationAccount = NULL;

    time_t now = time(NULL);
    tm local_time;
    localtime_s(&local_time, &now);
    string date = to_string(local_time.tm_mday) + "/" + to_string(1 + local_time.tm_mon) + "/" + to_string(1900 + local_time.tm_year);



    while (currentUser != NULL)
    {
        account* currentAccount = currentUser->acct;

        while (currentAccount != NULL)
        {
            if (currentAccount->IBAN == sourceIBAN)
            {
                sourceAccount = currentAccount;
            }
            if (currentAccount->IBAN == destinationIBAN)
            {
                destinationAccount = currentAccount;
            }
            currentAccount = currentAccount->next;
        }

        if (sourceAccount && destinationAccount)
        {
            break;
        }

        currentUser = currentUser->next;
    }

    if (sourceAccount == NULL || destinationAccount == NULL)
    {
        cout << "One or both accounts not found." << endl;
        return false;
    }

    if (sourceAccount->balance < amount)
    {
        cout << "Insufficient balance in source account." << endl;
        return false;
    }

    double totalWithdrawnThisMonth = 0.0;

    transaction* txn = sourceAccount->txn;
    while (txn) {
        tm txnTime = parseDate(txn->date);
        if (txnTime.tm_year == local_time.tm_year && txnTime.tm_mon == local_time.tm_mon && txn->amount < 0) {
            totalWithdrawnThisMonth += -txn->amount;
        }
        txn = txn->next;
    }

    if (totalWithdrawnThisMonth + amount > sourceAccount->limitWithdrawPerMonth) {
        cout << "Transfer exceeds the monthly withdrawal limit for the source account." << endl;
        return false;
    }

    double totalDepositedToday = 0.0;
    txn = destinationAccount->txn;
    while (txn) {
        tm txnTime = parseDate(txn->date);
        if (txnTime.tm_year == local_time.tm_year && txnTime.tm_mon == local_time.tm_mon &&
            txnTime.tm_mday == local_time.tm_mday && txn->amount > 0) {
            totalDepositedToday += txn->amount;
        }
        txn = txn->next;
    }

    if (totalDepositedToday + amount > destinationAccount->limitDepositPerDay) {
        cout << "Transfer exceeds the daily deposit limit for the destination account." << endl;
        return false;
    }


    if (sourceAccount->currency != destinationAccount->currency)
    {
        double newAmount = convertCurrency(amount, sourceAccount->currency, destinationAccount->currency);

        sourceAccount->balance -= amount;

        destinationAccount->balance += newAmount;

        addTransaction(sourceAccount, date, -amount);

        addTransaction(destinationAccount, date, newAmount);
    }

    if (sourceAccount->currency == destinationAccount->currency)
    {
        sourceAccount->balance -= amount;

        destinationAccount->balance += amount;

        addTransaction(sourceAccount, date, -amount);

        addTransaction(destinationAccount, date, amount);
    }

    cout << "Transfer successful. " << amount << " transferred from " << sourceIBAN << " to " << destinationIBAN << endl;
    return true;
}




bool compareDates(const string& date1, const string& date2)
{
    tm tm1 = parseDate(date1);
    tm tm2 = parseDate(date2);
    return mktime(&tm1) < mktime(&tm2);
}

void sortTransactions(account* acct)
{
    if (!acct || !acct->txn || !acct->txn->next)
    {
        return;
    }

    transaction* head = acct->txn;
    bool swapped;

    do
    {
        swapped = false;
        transaction* current = head;
        transaction* prev = NULL;

        while (current && current->next)
        {
            if (!compareDates(current->date, current->next->date))
            {
                transaction* temp = current->next;
                current->next = temp->next;
                temp->next = current;

                if (prev != NULL)
                {
                    prev->next = temp;
                }
                else
                {
                    head = temp;
                }

                swapped = true;
                prev = temp;
            }
            else
            {
                prev = current;
                current = current->next;
            }
        }
    } while (swapped);

    acct->txn = head;
}

/* TASK:INSERTION SORT
void sortTransactions(account* acct)
{
    if (!acct || !acct->txn || !acct->txn->next)
    {
        return;
    }

    transaction* sorted = NULL;
    transaction* current = acct->txn;

    while (current!=NULL)
    {
        transaction* next = current->next;
        if (!sorted || compareDates(current->date, sorted->date))
        {
            current->next = sorted;
            sorted = current;
        }
        else
        {
            transaction* temp = sorted;
            while (temp->next!=NULL && compareDates(temp->next->date, current->date))
            {
                temp = temp->next;
            }
            current->next = temp->next;
            temp->next = current;
        }
        current = next;
    }

    acct->txn = sorted;
}
*/

/*MERGE SORTING
transaction* merge(transaction* left, transaction* right)
{
    if (!left) return right;
    if (!right) return left;

    if (compareDates(left->date, right->date))
    {
        left->next = merge(left->next, right);
        return left;
    }
    else
    {
        right->next = merge(left, right->next);
        return right;
    }
}

transaction* mergeSort(transaction* head)
{
    if (!head || !head->next)
        return head;

    // Split the linked list into two halves
    transaction* slow = head;
    transaction* fast = head->next;

    while (fast && fast->next)
    {
        slow = slow->next;
        fast = fast->next->next;
    }

    transaction* mid = slow->next;
    slow->next = NULL;

    // Recursively sort each half
    transaction* left = mergeSort(head);
    transaction* right = mergeSort(mid);

    // Merge the sorted halves
    return merge(left, right);
}

void sortTransactions(account* acct)
{
    if (!acct || !acct->txn)
        return;

    acct->txn = mergeSort(acct->txn);
}
*/

void sortAllTransactions(userList& list)
{
    user* currentUser = list.head;
    while (currentUser)
    {
        account* currentAccount = currentUser->acct;
        while (currentAccount)
        {
            sortTransactions(currentAccount);
            currentAccount = currentAccount->next;
        }
        currentUser = currentUser->next;
    }
}


bool createAccountForUser(userList& user_list, user* currentUser)
{

    string IBAN, accountName, currency;
    double balance, limitDepositPerDay, limitWithdrawPerMonth;

    cout << "Enter IBAN for the new account: ";
    cin >> IBAN;


    account* existingAccount = currentUser->acct;
    while (existingAccount != NULL)
    {
        if (existingAccount->IBAN == IBAN)
        {
            cout << "This user already has an account with IBAN: " << IBAN << endl;
            return false;
        }
        existingAccount = existingAccount->next;
    }

    cout << "Enter account name: ";
    cin.ignore();
    getline(cin, accountName);

    cout << "Enter the initial balance: ";
    cin >> balance;

    cout << "Enter the currency: ";
    cin >> currency;

    cout << "Enter the deposit limit per day: ";
    cin >> limitDepositPerDay;

    cout << "Enter the withdrawal limit per month: ";
    cin >> limitWithdrawPerMonth;


    account* newAccount = new account{ IBAN, accountName, balance, currency, limitDepositPerDay, limitWithdrawPerMonth, NULL, NULL };

    if (currentUser->acct == NULL)
    {
        currentUser->acct = newAccount;
    }
    else
    {
        account* lastAccount = currentUser->acct;
        while (lastAccount->next != NULL)
        {
            lastAccount = lastAccount->next;
        }
        lastAccount->next = newAccount;
    }

    cout << "Account with IBAN: " << IBAN << " successfully created for user " << currentUser->fname << " " << currentUser->lname << endl;
    return true;
}



void displayData(const userList& list)
{
    user* currentUser = list.head;

    while (currentUser != NULL)
    {
        cout << "-" << currentUser->userID << "," << currentUser->fname << "," << currentUser->lname << "\n";

        account* currentAccount = currentUser->acct;
        while (currentAccount != NULL)
        {
            cout << "#" << currentAccount->IBAN << "," << currentAccount->accountName
                << "," << currentAccount->balance << currentAccount->currency
                << "," << currentAccount->limitDepositPerDay << currentAccount->currency
                << "," << currentAccount->limitWithdrawPerMonth << currentAccount->currency << "\n";

            transaction* currentTransaction = currentAccount->txn;
            while (currentTransaction != NULL)
            {
                cout << "*" << currentTransaction->date << "," << currentTransaction->amount << currentAccount->currency << "\n";
                currentTransaction = currentTransaction->next;
            }
            currentAccount = currentAccount->next;
        }
        currentUser = currentUser->next;
    }
}

void deleteTransactionsBeforeDate(userList& list, const string& date)
{

    tm enteredDate;
    try
    {
        enteredDate = parseDate(date);
    }
    catch (const invalid_argument& e)
    {
        cout << "Invalid date format. Please use DD/MM/YYYY." << endl;
        return;
    }

    time_t now = time(NULL);
    tm currentDate;
    localtime_s(&currentDate, &now);

    if (mktime(&enteredDate) >= mktime(&currentDate))
    {
        cout << "Error: Entered date (" << date << ") must be earlier than the current date." << endl;
        return;
    }
    user* currentUser = list.head;
    while (currentUser)
    {
        account* currentAccount = currentUser->acct;

        while (currentAccount)
        {
            transaction* currentTransaction = currentAccount->txn;
            transaction* prevTransaction = NULL;

            while (currentTransaction)
            {
                transaction* nextTransaction = currentTransaction->next;


                if (compareDates(currentTransaction->date, date))
                {

                    if (prevTransaction != NULL)
                    {
                        prevTransaction->next = nextTransaction;
                    }
                    else
                    {
                        currentAccount->txn = nextTransaction;
                    }

                    delete currentTransaction;
                }

                else
                {
                    prevTransaction = currentTransaction;
                }
                currentTransaction = nextTransaction;
            }
            currentAccount = currentAccount->next;
        }
        currentUser = currentUser->next;
    }
    cout << "Transactions before " << date << " have been deleted.\n";
}



userList* parseData(string& filename) {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    ifstream infile(filename);
    if (!infile.is_open()) {
        cerr << "Error opening file!" << endl;
        return NULL;
    }

    userList* ulist = new userList{ NULL, NULL };
    user* currentUser = NULL;
    account* currentAccount = NULL;

    string line;
    while (getline(infile, line)) {
        if (line.empty()) continue;

        if (line[0] == '-') {
            stringstream ss(line.substr(1));
            string userIDStr, fname, lname;
            getline(ss, userIDStr, ',');
            getline(ss, fname, ',');
            getline(ss, lname, ',');

            int userID = stoi(userIDStr);

            user* newUser = new user{ userID, fname, lname, NULL, NULL, NULL };

            if (!ulist->head) {
                ulist->head = ulist->tail = newUser;
            }
            else {
                ulist->tail->next = newUser;
                newUser->previous = ulist->tail;
                ulist->tail = newUser;
            }
            currentUser = newUser;
        }
        else if (line[0] == '#') {
            stringstream ss(line.substr(1));
            string IBAN, accountName, currency;
            double balance, limitDepositPerDay, limitWithdrawPerMonth;

            getline(ss, IBAN, ',');
            getline(ss, accountName, ',');
            ss >> balance;
            getline(ss, currency, ',');
            ss >> limitDepositPerDay;
            getline(ss, currency, ',');
            ss >> limitWithdrawPerMonth;

            account* newAccount = new account{ IBAN, accountName, balance, currency, limitDepositPerDay, limitWithdrawPerMonth, NULL, NULL };

            if (!currentUser->acct) {
                currentUser->acct = newAccount;
            }
            else {
                account* temp = currentUser->acct;
                while (temp->next) temp = temp->next;
                temp->next = newAccount;
            }
            currentAccount = newAccount;

        }
        else if (line[0] == '*') {
            if (!currentAccount) {
                cerr << "Error: Transaction line encountered without an account." << endl;
                continue;
            }
            istringstream ss(line.substr(1)); 
            string date;
            double amount;

            getline(ss, date, ',');  

            ss >> amount;

            transaction* newTransaction = new transaction{ date, amount, NULL };

            if (!currentAccount->txn) {
                currentAccount->txn = newTransaction;
            }
            else {
                transaction* temp = currentAccount->txn;
                while (temp->next) temp = temp->next;
                temp->next = newTransaction;
            }
        }
    }

    infile.close();
    return ulist;
}


void writeDataToFile(const userList& user_list, const string& filepath)
{
    ofstream file(filepath);
    if (!file)
    {
        cout << "Error opening file at " << filepath << endl;
        return;
    }

    unordered_set<string> writtenIBANs;

    user* currentUser = user_list.head;
    while (currentUser != NULL)
    {

        file << "-" << currentUser->userID << "," << currentUser->fname << "," << currentUser->lname << endl;

        account* currentAccount = currentUser->acct;
        while (currentAccount != NULL)
        {
            if (writtenIBANs.find(currentAccount->IBAN) == writtenIBANs.end())
            {

                file << "#" << currentAccount->IBAN << "," << currentAccount->accountName << ","
                    << currentAccount->balance << currentAccount->currency << ","
                    << currentAccount->limitDepositPerDay << currentAccount->currency << ","
                    << currentAccount->limitWithdrawPerMonth << currentAccount->currency << endl;


                writtenIBANs.insert(currentAccount->IBAN);

            }

            transaction* currentTransaction = currentAccount->txn;
            while (currentTransaction != NULL)
            {
                file << "*" << currentTransaction->date << "," << currentTransaction->amount << currentAccount->currency << endl;
                currentTransaction = currentTransaction->next;
            }
            currentAccount = currentAccount->next;
        }
        currentUser = currentUser->next;
    }
    file.close();
}




int main()
{
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    string filepath = "User.txt";
    userList user_list;
    InitializeUsers(user_list);
    user_list = *parseData(filepath);

    int choice;
    do
    {
        cout << "This is the Bank Account Manager" << endl << endl;
        cout << "Press 1 to display all the accounts" << endl;
        cout << "Press 2 to transfer from one account to another" << endl;
        cout << "Press 3 to add a new user" << endl;
        cout << "Press 4 to create a new account for an existing user" << endl;
        cout << "Press 5 to sort transactions by date" << endl;
        cout << "Press 6 to delete transactions before a specified date" << endl;
        cout << "Press 0 to exit" << endl;
        cin >> choice;

        if (choice == 1)
        {
            displayData(user_list);
            cout << endl;
        }

        if (choice == 2)
        {
            displayData(user_list);
            cout << endl;
            string Acc1, Acc2;
            double amount;
            cout << "Please enter the account you wish to transfer from: ";
            cin >> Acc1;
            cout << "Please enter the account you wish to transfer to: ";
            cin >> Acc2;
            cout << "Please enter the amount you wish to transfer : ";
            cin >> amount;
            transferAmount(user_list, amount, Acc1, Acc2);
            cout << endl;
        }

        if (choice == 3)
        {
            cout << "\nAdd a new user:\n";
            int userID;
            string fname, lname;
            cout << "Enter new user ID: ";
            cin >> userID;
            cout << "Enter first name: ";
            cin >> fname;
            cout << "Enter last name: ";
            cin >> lname;
            addUser(user_list, userID, fname, lname);
            cout << "New user added successfully.\n";
        }

        if (choice == 4)
        {
            displayData(user_list);
            int userID;
            cout << "Enter the user ID of the user you want to add an account for: ";
            cin >> userID;
            user* currentUser = user_list.head;

            while (currentUser != NULL && currentUser->userID != userID)
            {
                currentUser = currentUser->next;
            }

            if (currentUser == NULL)
            {
                cout << "User with ID " << userID << " not found.\n";
            }
            else
            {
                if (createAccountForUser(user_list, currentUser))
                {
                    cout << "Account successfully added for user " << currentUser->fname << " " << currentUser->lname << ".\n";
                }
                else
                {
                    cout << "Failed to create account. Please and try again.\n";
                }
            }
        }

        if (choice == 5)
        {
            sortAllTransactions(user_list);
            displayData(user_list);
            cout << endl;
        }

        if (choice == 6)
        {
            string date;
            cout << "Enter the date (DD/MM/YYYY) to delete transactions before: ";
            cin >> date;
            deleteTransactionsBeforeDate(user_list, date);
        }

    } while (choice != 0);

    writeDataToFile(user_list, filepath);

    return 0;
}
