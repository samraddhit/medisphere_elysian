#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define MAX 50
#define NSLOTS 7
#define DATA_FILE "medisphere.dat"

struct User {
    int id, role;                         /* role 1 = patient, 3 = admin */
    char name[50], email[50], pass[30];
};

struct Doctor {
    int id;
    char name[50], spec[50];
};
struct Appointment {
    int id, patient, doctor, active;      /* active 1 = booked, 0 = cancelled */
    char date[15], time[10];
};

struct User users[MAX];
struct Doctor doctors[MAX];
struct Appointment app[MAX];

int ucount = 0, dcount = 0, acount = 0;

/* every doctor can be booked in these one-hour slots */
const char *slots[NSLOTS] = {"09:00", "10:00", "11:00", "12:00", "14:00", "15:00", "16:00"};
/* reads a number safely; returns -1 if the user typed something else */
int getInt() {
    int x, ch;
    int r = scanf("%d", &x);
    if (r == EOF) exit(0);
    if (r != 1) {
        while ((ch = getchar()) != '\n' && ch != EOF);   /* throw away the bad input */
        return -1;
    }
    return x;
}
/* checks the format DD-MM-YYYY */
int validDate(char *d) {
    if (strlen(d) != 10 || d[2] != '-' || d[5] != '-') return 0;
    for (int i = 0; i < 10; i++)
        if (i != 2 && i != 5 && (d[i] < '0' || d[i] > '9')) return 0;
    int day = atoi(d), month = atoi(d + 3);
    return day >= 1 && day <= 31 && month >= 1 && month <= 12;
}

int findUser(int id) {
    for (int i = 0; i < ucount; i++) if (users[i].id == id) return i;
    return -1;                                            /* -1 = not found */
}

int findDoctor(int id) {
    for (int i = 0; i < dcount; i++) if (doctors[i].id == id) return i;
    return -1;
}
/* ---------- saving and loading ---------- */

/* each list is stored as: how many items, then the items */
void saveList(FILE *f, void *list, int size, int count) {
    fwrite(&count, sizeof(int), 1, f);
    fwrite(list, size, count, f);
}

void loadList(FILE *f, void *list, int size, int *count) {
    if (fread(count, sizeof(int), 1, f) != 1 || *count < 0 || *count > MAX) {
        *count = 0;                                       /* file missing or damaged */
        return;
    }
    if (fread(list, size, *count, f) != (size_t)*count) *count = 0;
}

void saveData() {
    FILE *f = fopen(DATA_FILE, "wb");
    if (f == NULL) return;
    saveList(f, users, sizeof(struct User), ucount);
    saveList(f, doctors, sizeof(struct Doctor), dcount);
    saveList(f, app, sizeof(struct Appointment), acount);
    fclose(f);
}

void loadData() {
    FILE *f = fopen(DATA_FILE, "rb");
    if (f == NULL) return;
    loadList(f, users, sizeof(struct User), &ucount);
    loadList(f, doctors, sizeof(struct Doctor), &dcount);
    loadList(f, app, sizeof(struct Appointment), &acount);
    fclose(f);
}
/* ---------- registration and login (Milestone 2) ---------- */

void registerPatient() {
    if (ucount >= MAX) { printf("User list is full.\n"); return; }

    printf("\nName: ");
    scanf(" %49[^\n]", users[ucount].name);

    printf("Email: ");
    scanf("%49s", users[ucount].email);
    for (int i = 0; i < ucount; i++) {
        if (strcmp(users[i].email, users[ucount].email) == 0) {
            printf("This email is already registered.\n");
            return;
        }
    }

    printf("Password: ");
    scanf("%29s", users[ucount].pass);

    users[ucount].id = ucount + 1;
    users[ucount].role = 1;
    ucount++;
    saveData();

    printf("Registration Successful!\n");
}

int login() {
    char email[50], pass[30];

    printf("\nEmail: ");
    scanf("%49s", email);

    printf("Password: ");
    scanf("%29s", pass);

    for (int i = 0; i < ucount; i++) {
        if (!strcmp(users[i].email, email) &&
            !strcmp(users[i].pass, pass))
            return i;
    }

    printf("Invalid Login!\n");
    return -1;
}

/* ---------- doctors ---------- */

void addDoctor() {
    if (dcount >= MAX) { printf("Doctor list is full.\n"); return; }
    doctors[dcount].id = dcount + 1;

    printf("\nDoctor Name: ");
    scanf(" %49[^\n]", doctors[dcount].name);

    printf("Specialization: ");
    scanf(" %49[^\n]", doctors[dcount].spec);

    dcount++;
    saveData();
    printf("Doctor Added Successfully!\n");
}

void showDoctors() {
    printf("\n--- Doctors ---\n");
    if (dcount == 0) printf("No doctors yet.\n");

    for (int i = 0; i < dcount; i++)
        printf("%d. %s - %s\n",
               doctors[i].id,
               doctors[i].name,
               doctors[i].spec);
}

/* ---------- scheduling (Milestone 3) ---------- */

/* is this doctor already booked on that date in slot number s? */
int slotTaken(int doctorId, char *date, int s) {
    for (int i = 0; i < acount; i++)
        if (app[i].active && app[i].doctor == doctorId &&
            strcmp(app[i].date, date) == 0 && strcmp(app[i].time, slots[s]) == 0)
            return 1;
    return 0;
}

/* is this patient already booked with any doctor at that date and slot? */
int patientBusy(int patientId, char *date, int s) {
    if (patientId == 0) return 0;                         /* 0 = no patient given */
    for (int i = 0; i < acount; i++)
        if (app[i].active && app[i].patient == patientId &&
            strcmp(app[i].date, date) == 0 && strcmp(app[i].time, slots[s]) == 0)
            return 1;
    return 0;
}

/* first free slot of doctor number d on a date (-1 if the day is full) */
int nextFreeSlot(int d, char *date, int patientId) {
    for (int s = 0; s < NSLOTS; s++)
        if (!slotTaken(doctors[d].id, date, s) && !patientBusy(patientId, date, s))
            return s;
    return -1;
}

/* how many active appointments does this doctor have on a date? */
int countBooked(int doctorId, char *date) {
    int count = 0;
    for (int i = 0; i < acount; i++)
        if (app[i].active && app[i].doctor == doctorId && strcmp(app[i].date, date) == 0)
            count++;
    return count;
}

/* saves one appointment and returns its id */
int addAppointment(int patient, int doctorId, char *date, int s) {
    app[acount].id = acount + 1;
    app[acount].patient = patient;
    app[acount].doctor = doctorId;
    app[acount].active = 1;
    strcpy(app[acount].date, date);
    strcpy(app[acount].time, slots[s]);
    acount++;
    saveData();
    return acount;
}

/* normal booking: the patient picks the doctor and the slot */
void bookAppointment(int patient) {
    char date[15];
    int id, d, s;

    if (acount >= MAX) { printf("Appointment list is full.\n"); return; }
    showDoctors();
    if (dcount == 0) return;

    printf("\nDoctor ID: ");
    id = getInt();
    d = findDoctor(id);
    if (d == -1) { printf("No such doctor.\n"); return; }

    printf("Date (DD-MM-YYYY): ");
    scanf("%14s", date);
    if (!validDate(date)) { printf("Invalid date. Use DD-MM-YYYY.\n"); return; }

    printf("\nSlots on %s:\n", date);
    for (int i = 0; i < NSLOTS; i++)
        printf("%d. %s %s\n", i + 1, slots[i], slotTaken(id, date, i) ? "(booked)" : "(free)");

    printf("Choose slot: ");
    s = getInt() - 1;
    if (s < 0 || s >= NSLOTS) { printf("Invalid slot.\n"); return; }

    if (slotTaken(id, date, s)) { printf("That slot is already booked.\n"); return; }
    if (patientBusy(patient, date, s)) { printf("You already have an appointment at that time.\n"); return; }

    addAppointment(patient, id, date, s);
    printf("Appointment Booked! (%s, %s at %s)\n", doctors[d].name, date, slots[s]);
}

/* shows the next free slot of one doctor */
void suggestSlot() {
    char date[15];
    int d, s;

    showDoctors();
    if (dcount == 0) return;
    printf("\nDoctor ID: ");
    d = findDoctor(getInt());
    if (d == -1) { printf("No such doctor.\n"); return; }

    printf("Date (DD-MM-YYYY): ");
    scanf("%14s", date);
    if (!validDate(date)) { printf("Invalid date. Use DD-MM-YYYY.\n"); return; }

    s = nextFreeSlot(d, date, 0);
    if (s == -1) printf("%s is fully booked on %s.\n", doctors[d].name, date);
    else printf("Next free slot: %s at %s on %s.\n", doctors[d].name, slots[s], date);
}
/* smart booking. Rule 1: choose the doctor with the EARLIEST free slot.
   Rule 2: if two doctors are free at the same time, choose the one with
   fewer appointments that day (so the work is shared). */
void autoBook(int patient) {
    char date[15];
    int bestD = -1, bestS = NSLOTS, bestLoad = MAX + 1;

    if (acount >= MAX) { printf("Appointment list is full.\n"); return; }
    if (dcount == 0) { printf("No doctors yet.\n"); return; }

    printf("\nDate (DD-MM-YYYY): ");
    scanf("%14s", date);
    if (!validDate(date)) { printf("Invalid date. Use DD-MM-YYYY.\n"); return; }

    for (int i = 0; i < dcount; i++) {
        int s = nextFreeSlot(i, date, patient);
        if (s == -1) continue;                            /* this doctor is full */
        int load = countBooked(doctors[i].id, date);
        if (s < bestS || (s == bestS && load < bestLoad)) {
            bestD = i; bestS = s; bestLoad = load;
        }
    }
    if (bestD == -1) { printf("No doctor has a free slot on %s.\n", date); return; }

    addAppointment(patient, doctors[bestD].id, date, bestS);
    printf("Smart booking done! %s at %s on %s.\n", doctors[bestD].name, slots[bestS], date);
}

void showAppointments(int patient) {
    int found = 0;
    printf("\n--- My Appointments ---\n");

    for (int i = 0; i < acount; i++) {
        if (app[i].patient == patient) {
            int d = findDoctor(app[i].doctor);
            printf("ID:%d  %s  Date:%s  Time:%s  %s\n",
                   app[i].id, d == -1 ? "?" : doctors[d].name,
                   app[i].date, app[i].time,
                   app[i].active ? "Booked" : "Cancelled");
            found = 1;
        }
    }
    if (!found) printf("No appointments yet.\n");
}

void cancelAppointment(int patient) {
    printf("\nAppointment ID to cancel: ");
    int id = getInt();

    if (id < 1 || id > acount || app[id - 1].patient != patient) {
        printf("No such appointment in your list.\n");
        return;
    }
    if (!app[id - 1].active) { printf("Already cancelled.\n"); return; }

    app[id - 1].active = 0;
    saveData();
    printf("Appointment %d cancelled. The slot is free again.\n", id);
}
/* ---------- admin: reports and system check (Milestone 3) ---------- */

void showAllAppointments() {
    printf("\n--- All Appointments ---\n");
    if (acount == 0) printf("No appointments yet.\n");

    for (int i = 0; i < acount; i++) {
        int p = findUser(app[i].patient), d = findDoctor(app[i].doctor);
        printf("ID:%d  %s  with %s  %s %s  %s\n", app[i].id,
               p == -1 ? "?" : users[p].name, d == -1 ? "?" : doctors[d].name,
               app[i].date, app[i].time, app[i].active ? "Booked" : "Cancelled");
    }
}

void report() {
    int patients = 0, booked = 0, cancelled = 0;

    for (int i = 0; i < ucount; i++) if (users[i].role == 1) patients++;
    for (int i = 0; i < acount; i++) {
        if (app[i].active) booked++; else cancelled++;
    }

    printf("\n===== SYSTEM REPORT =====\n");
    printf("Patients registered   : %d\n", patients);
    printf("Doctors               : %d\n", dcount);
    printf("Appointments booked   : %d\n", booked);
    printf("Appointments cancelled: %d\n", cancelled);

    printf("\nDoctor               Booked  Cancelled\n");
    for (int j = 0; j < dcount; j++) {
        int b = 0, c = 0;
        for (int i = 0; i < acount; i++) {
            if (app[i].doctor != doctors[j].id) continue;
            if (app[i].active) b++; else c++;
        }
        printf("%-20.20s %-7d %d\n", doctors[j].name, b, c);
    }
}

/* data consistency: returns the number of problems found */
int checkData() {
    int problems = 0;

    for (int i = 0; i < ucount; i++)                      /* duplicate emails */
        for (int j = i + 1; j < ucount; j++)
            if (strcmp(users[i].email, users[j].email) == 0) {
                printf("PROBLEM: users %d and %d share an email.\n", users[i].id, users[j].id);
                problems++;
            }

    for (int i = 0; i < acount; i++) {
        int p = findUser(app[i].patient), validSlot = 0;
        if (p == -1 || users[p].role != 1) {
            printf("PROBLEM: appointment %d has an unknown patient.\n", app[i].id);
            problems++;
        }
        if (findDoctor(app[i].doctor) == -1) {
            printf("PROBLEM: appointment %d has an unknown doctor.\n", app[i].id);
            problems++;
        }
        for (int s = 0; s < NSLOTS; s++)
            if (strcmp(app[i].time, slots[s]) == 0) validSlot = 1;
        if (!validSlot) {
            printf("PROBLEM: appointment %d has an invalid time.\n", app[i].id);
            problems++;
        }
        if (!app[i].active) continue;
        for (int j = i + 1; j < acount; j++)              /* double booking */
            if (app[j].active && app[j].doctor == app[i].doctor &&
                strcmp(app[j].date, app[i].date) == 0 && strcmp(app[j].time, app[i].time) == 0) {
                printf("PROBLEM: appointments %d and %d use the same slot.\n", app[i].id, app[j].id);
                problems++;
            }
    }
    return problems;
}

void systemCheck() {
    volatile int sink = 0;                                /* stops the loop being skipped */

    printf("\n===== SYSTEM CHECK =====\n");
    int problems = checkData();
    if (problems == 0) printf("Data consistency: PASSED (%d appointments checked)\n", acount);
    else printf("Data consistency: FAILED (%d problems)\n", problems);

    if (dcount == 0) { printf("Add a doctor to run the timing test.\n"); return; }

    clock_t start = clock();
    for (int i = 0; i < 10000; i++) sink += nextFreeSlot(0, "01-01-2030", 0);
    double searchUs = (double)(clock() - start) * 1000000.0 / CLOCKS_PER_SEC / 10000;
    printf("Find next free slot : %.2f microseconds on average\n", searchUs);

    start = clock();
    for (int i = 0; i < 20; i++) saveData();
    double saveMs = (double)(clock() - start) * 1000.0 / CLOCKS_PER_SEC / 20;
    printf("Save data to file   : %.2f milliseconds on average\n", saveMs);
}
/* ---------- menus ---------- */

void adminMenu() {
    int c;
    do {
        printf("\n--- ADMIN ---\n");
        printf("1. Add Doctor\n2. View Doctors\n3. All Appointments\n");
        printf("4. Report\n5. System Check\n6. Logout\n");
        printf("Choice: ");
        c = getInt();

        if (c == 1) addDoctor();
        else if (c == 2) showDoctors();
        else if (c == 3) showAllAppointments();
        else if (c == 4) report();
        else if (c == 5) systemCheck();
        else if (c != 6) printf("Invalid choice.\n");
    } while (c != 6);
}

void patientMenu(int patient) {
    int c;
    do {
        printf("\n--- PATIENT ---\n");
        printf("1. View Doctors\n2. Book Appointment\n3. Smart Booking\n");
        printf("4. Next Free Slot\n5. My Appointments\n6. Cancel Appointment\n7. Logout\n");
        printf("Choice: ");
        c = getInt();

        if (c == 1) showDoctors();
        else if (c == 2) bookAppointment(patient);
        else if (c == 3) autoBook(patient);
        else if (c == 4) suggestSlot();
        else if (c == 5) showAppointments(patient);
        else if (c == 6) cancelAppointment(patient);
        else if (c != 7) printf("Invalid choice.\n");
    } while (c != 7);
}

int main() {
    int choice, user;

    loadData();

    /* Default Admin (created only on the very first run) */
    if (ucount == 0) {
        users[0].id = 1;
        strcpy(users[0].name, "Admin");
        strcpy(users[0].email, "admin@gmail.com");
        strcpy(users[0].pass, "admin123");
        users[0].role = 3;
        ucount = 1;
        saveData();
    }

    while (1) {
        printf("\n====== MEDISPHERE ======\n");
        printf("1. Patient Registration\n");
        printf("2. Login\n");
        printf("3. Exit\n");
        printf("Choice: ");
        choice = getInt();

        if (choice == 1)
            registerPatient();

        else if (choice == 2) {
            user = login();
            if (user == -1)
                continue;

            if (users[user].role == 3)
                adminMenu();
            else
                patientMenu(users[user].id);
        }
        else if (choice == 3)
            break;
        else
            printf("Invalid choice.\n");
    }

    return 0;
}