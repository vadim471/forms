#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>


typedef struct _person {
    int age;
    int exp;
    char name[10 + 1];
    char surname[20 + 1];
    //struct _person* another;
} person;

void file_edit(char* path){
    unsigned char buffer[4] = {0, 0, 0, 0};
    int file = open(path, O_WRONLY | O_CREAT, S_IRWXU | S_IRGRP | S_IROTH);
    if (file == -1) {
        printf("%s\n", strerror(errno));
        return;
    }
    write(file, 0, sizeof(int));
    close(file);
}
void personView(person human) { //view person
    printf("Name: %s\nSurname: %s\nAge: %d\nExperience: %d\n", human.name, human.surname, human.age, human.exp);
}

person *personAdd() {
    person *new_person = (person *) malloc(sizeof(person));
    memset(new_person, 0, sizeof(person));

    printf("Введите возраст\n");
    scanf("%d[^\n]", &new_person->age);
    printf("Введите стаж работы\n");
    scanf("%d[^\n]", &new_person->exp);
    printf("Введите имя\n");
    scanf("%s[^\n]", new_person->name);
    new_person->name[sizeof new_person->name - 1] = '\0';
    printf("Введите фамилию\n");
    scanf("%s[^\n]", new_person->surname);
    new_person->surname[sizeof new_person->surname - 1] = '\0';

    return new_person;
}

int personFile(){ //add person to file
    int amount_form = 0;
    unsigned char buffer[4];
    person *new_person = personAdd();

    file_edit("../forms.bin");

    int file = open("../forms.bin", O_RDWR);
    if (file == -1) {
        printf("%s\n", strerror(errno));
        return 1;
    }

    read(file, buffer, sizeof(buffer));
    int err = sscanf((char *)buffer, "%d", &amount_form);
    if (err < 0){
        amount_form = 0;
    }
    unsigned int byte_next_form = sizeof(int) + sizeof(person) * amount_form;
    amount_form++;
    struct flock fl;

    lseek(file, byte_next_form, SEEK_SET);

    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_CUR;
    fl.l_len = sizeof(person);
    if (fcntl(file, F_SETLK, &fl) == -1) {
        printf("%s\n", strerror(errno));
        return 2;
    }
    fcntl(file, F_SETLK, &fl);

    write(file, new_person, sizeof(person));
    close(file);
    file = open("../forms.bin", O_RDWR);
    lseek(file, 0, SEEK_SET);
    memset(&fl, 0, sizeof(fl));
    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_CUR;
    fl.l_len = sizeof(buffer);
    sprintf((char *)buffer, "%d", amount_form);
    write(file, buffer, sizeof(buffer));
    close(file);
    getchar();
    return 0;
}

person *getPerson(int id) { //from file to person
    unsigned int byte = sizeof(int) + id * sizeof(person);

    file_edit("../forms.bin");

    int file = open("../forms.bin", O_RDONLY);
    struct flock fl;
    memset(&fl, 0, sizeof(fl));

    // edit lock TODO
    if (file == -1) {
        perror("File does not exist");
        return NULL;
    }
    int seek = lseek(file, byte, SEEK_SET);
    if (seek == byte - 1) {
        perror("Person does not exist");
        return NULL;
    }
    fl.l_type = F_RDLCK;
    fl.l_whence = SEEK_CUR;
    fl.l_len = sizeof(person);
    if (fcntl(file, F_SETLK, &fl) == -1) {
        printf("%s\n", strerror(errno));
        return NULL;
    }
    fcntl(file, F_SETLK, &fl);

    person *req_person = malloc(sizeof(person));

    read(file, req_person, sizeof(person));

    close(file);
    return req_person;
}

void personEdit(int id){
    person *curr_person = getPerson(id);
    personView(*curr_person);
    unsigned int byte = sizeof(int) + sizeof(person) * id;
    int file = open("../forms.bin", O_RDWR);
    struct flock fl;

    if (file == -1) {
        printf("%s\n", strerror(errno));
        return;
    }

    int seek = lseek(file, byte, SEEK_SET);
    if (seek == byte - 1) {
        perror("Person does not exist");
        return;
    }

    fl.l_whence = SEEK_CUR;
    fl.l_len = sizeof(person);
    if (fcntl(file, F_SETLK, &fl) == -1) {
        printf("%s\n", strerror(errno));
        return;
    }
    fcntl(file, F_SETLK, &fl);

    curr_person = personAdd();

    write(file, curr_person, sizeof(person));
    close(file);

}
int getFormIndex() {
    printf("Введите номер анкеты, над которой будете производить операции: ");
    int formIndex;
    scanf("%d[^\n]", &formIndex);
    return formIndex - 1;
}
int getOperationIndex() {
    printf("Введите номер операции, которую хотели бы выполнить:\n");
    printf("1. Чтение анкеты;\n");
    printf("2. Создание новой анкеты;\n");
    printf("3. Редактирование анкеты;\n");
    printf("Введите номер выбранной операции: ");
    int operation_index;
    scanf("%d[^\n]", &operation_index);
    return operation_index;
}
int main() {
    char flag = 0;
    file_edit("asd.bin");
    while (1) {

        int form_index = getFormIndex();
        if (form_index < 0) {
            printf("Нет анкеты с таким номером\n");
            continue;
        }

        int func_index = getOperationIndex();
        switch (func_index) {
            case 1: {
                person *req_person = getPerson(form_index);
                if (req_person == NULL) {
                    printf("Reading error");
                } else {
                    personView(*req_person);
                }
                break;
            }
            case 2: {
                int result = personFile();
                if (result == 1)
                    printf("File open error");
                else if (result == 2){
                    printf("Writing error");
                }
                break;
            }
            case 3: {
                personEdit(form_index);
                break;
            }
            default:
                flag = 1;
                break;
        }
        if (flag) {
            break;
        }
    }
    return 0;
}
