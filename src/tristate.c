// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2023 Bardia Moshiri <fakeshell@bardia.tech>

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <linux/input.h>
#include <libnotify/notify.h>

#define MAX_CONFIG_LINE_LENGTH 512
#define CONFIG_PATH "/usr/lib/droidian/device/action-on-key.conf"

char DEVICE_FILE[MAX_CONFIG_LINE_LENGTH];
char NAME[MAX_CONFIG_LINE_LENGTH];
char COMMAND_1[MAX_CONFIG_LINE_LENGTH];
char COMMAND_2[MAX_CONFIG_LINE_LENGTH];
char COMMAND_3[MAX_CONFIG_LINE_LENGTH];
char NOTIFICATION_TITLE_1[MAX_CONFIG_LINE_LENGTH];
char NOTIFICATION_TITLE_2[MAX_CONFIG_LINE_LENGTH];
char NOTIFICATION_TITLE_3[MAX_CONFIG_LINE_LENGTH];
char NOTIFICATION_MESSAGE_1[MAX_CONFIG_LINE_LENGTH];
char NOTIFICATION_MESSAGE_2[MAX_CONFIG_LINE_LENGTH];
char NOTIFICATION_MESSAGE_3[MAX_CONFIG_LINE_LENGTH];
int KEY_CODE_1;
int KEY_CODE_2;
int KEY_CODE_3;

typedef struct node {
    char command[100];
    struct node *next;
} Node;

Node *front = NULL;
Node *rear = NULL;
pthread_mutex_t lock;
pthread_cond_t cond;

void enqueue(char* command) {
    Node *temp = (Node*)malloc(sizeof(Node));
    strncpy(temp->command, command, sizeof(temp->command));
    temp->next = NULL;

    pthread_mutex_lock(&lock);

    if (rear == NULL) {
        front = rear = temp;
    } else {
        rear->next = temp;
        rear = temp;
    }

    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&lock);
}

void* worker_thread(void *arg) {
    while (1) {
        pthread_mutex_lock(&lock);

        while (front == NULL) {
            pthread_cond_wait(&cond, &lock);
        }

        Node *temp = front;
        front = front->next;

        if (front == NULL) {
            rear = NULL;
        }

        pthread_mutex_unlock(&lock);

        // Execute the system command
        system(temp->command);

        free(temp);
    }

    return NULL;
}

void init_queue_and_worker() {
    pthread_t tid;
    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&cond, NULL);

    // Start the worker thread
    pthread_create(&tid, NULL, worker_thread, NULL);
}

void readconfig() {
    FILE *file = fopen(CONFIG_PATH, "r");
    if (file == NULL) {
        g_print("Error: Could not open config file\n");
        exit(1);
    }

    char line[MAX_CONFIG_LINE_LENGTH];
    while (fgets(line, sizeof(line), file)) {
        char *key = strtok(line, "=");
        char *value = strtok(NULL, "\n");

        if (strcmp(key, "DEVICE_FILE") == 0) {
            strcpy(DEVICE_FILE, value);
        } else if (strcmp(key, "NAME") == 0) {
            strcpy(NAME, value);
        } else if (strcmp(key, "KEY_CODE_1") == 0) {
            KEY_CODE_1 = atoi(value);
        } else if (strcmp(key, "KEY_CODE_2") == 0) {
            KEY_CODE_2 = atoi(value);
        } else if (strcmp(key, "KEY_CODE_3") == 0) {
            KEY_CODE_3 = atoi(value);
        } else if (strcmp(key, "COMMAND_1") == 0) {
            strcpy(COMMAND_1, value);
        } else if (strcmp(key, "COMMAND_2") == 0) {
            strcpy(COMMAND_2, value);
        } else if (strcmp(key, "COMMAND_3") == 0) {
            strcpy(COMMAND_3, value);
        } else if (strcmp(key, "NOTIFICATION_TITLE_1") == 0) {
            strcpy(NOTIFICATION_TITLE_1, value);
        } else if (strcmp(key, "NOTIFICATION_TITLE_2") == 0) {
            strcpy(NOTIFICATION_TITLE_2, value);
        } else if (strcmp(key, "NOTIFICATION_TITLE_3") == 0) {
            strcpy(NOTIFICATION_TITLE_3, value);
        } else if (strcmp(key, "NOTIFICATION_MESSAGE_1") == 0) {
            strcpy(NOTIFICATION_MESSAGE_1, value);
        } else if (strcmp(key, "NOTIFICATION_MESSAGE_2") == 0) {
            strcpy(NOTIFICATION_MESSAGE_2, value);
        } else if (strcmp(key, "NOTIFICATION_MESSAGE_3") == 0) {
            strcpy(NOTIFICATION_MESSAGE_3, value);
        }
    }

    fclose(file);
}

void send_notification(char *title, char *message) {
    NotifyNotification *notification = notify_notification_new(title, message, NULL);
    notify_notification_set_timeout(notification, 2000);

    notify_notification_set_hint_string(notification, "category", "device");
    notify_notification_set_urgency(notification, NOTIFY_URGENCY_LOW);

    notify_notification_set_hint(notification, "transient", g_variant_new_boolean(TRUE));

    notify_notification_show(notification, NULL);
    g_object_unref(notification);
}

int main(int argc, char *argv[]) {
    readconfig();

    struct input_event ev;

    if (!notify_init(NAME)) {
        g_print("Error: Failed to initialize libnotify\n");
        return 1;
    }

    init_queue_and_worker();

    while (1) {
        int fd_device = open(DEVICE_FILE, O_RDONLY);
        if (fd_device == -1) {
            perror("Cannot access input device");
            exit(EXIT_FAILURE);
        }
        read(fd_device, &ev, sizeof(struct input_event));
        if (ev.type == EV_KEY && ev.value == 1) {
            if (ev.code == KEY_CODE_1) {
                enqueue(COMMAND_1);
                send_notification(NOTIFICATION_TITLE_1, NOTIFICATION_MESSAGE_1);
            } else if (ev.code == KEY_CODE_2) {
                enqueue(COMMAND_2);
                send_notification(NOTIFICATION_TITLE_2, NOTIFICATION_MESSAGE_2);
            } else if (ev.code == KEY_CODE_3) {
                enqueue(COMMAND_3);
                send_notification(NOTIFICATION_TITLE_3, NOTIFICATION_MESSAGE_3);
            }
        }
        close(fd_device);
    }

    return 0;
}
