#ifndef ADMIN_H
#define ADMIN_H

#include <string>
#include <iostream>
#include <memory>

#include "User.h"
#include "Task.h"

// Forward declaration to avoid circular dependency
class TaskScheduler;

// Admin inherits from abstract User - Inheritance principle
class Admin : public User {
private:
    TaskScheduler* scheduler;

public:
    Admin(int id, const std::string& username, const std::string& password)
        : User(id, username, password), scheduler(nullptr) {}

    void setScheduler(TaskScheduler* s) { scheduler = s; }

    std::string getRole() const override {
        return "Admin";
    }

    // Polymorphism - override pure virtual function
    void showDashboard() override;
};

#endif // ADMIN_H