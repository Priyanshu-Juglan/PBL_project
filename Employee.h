#ifndef EMPLOYEE_H
#define EMPLOYEE_H

#include <queue>
#include <vector>
#include <unordered_map>
#include <string>
#include <iostream>
#include <algorithm>

#include "User.h"
#include "Task.h"

// Forward declaration to avoid circular dependency
class TaskScheduler;

// Employee inherits from abstract User - Inheritance principle
class Employee : public User {
private:
    // Employee Assigned Tasks - FIFO queue
    std::queue<Task> assignedTasks;

    // Status Buckets - O(1) categorization
    std::unordered_map<TaskStatus, std::vector<Task>> statusBuckets;

    // Pointer to scheduler for master record sync
    TaskScheduler* scheduler;

public:
    Employee(int id, const std::string& username, const std::string& password)
        : User(id, username, password), scheduler(nullptr) {
        // Initialize all status buckets
        statusBuckets[TaskStatus::ACCEPTED] = {};
        statusBuckets[TaskStatus::WORKING] = {};
        statusBuckets[TaskStatus::INCOMPLETE] = {};
        statusBuckets[TaskStatus::FAILED] = {};
        statusBuckets[TaskStatus::COMPLETED] = {};
    }

    // Polymorphism - override pure virtual functions
    void showDashboard() override;

    std::string getRole() const override {
        return "Employee";
    }

    std::string getPassword() const { return password; }

    void setScheduler(TaskScheduler* s) { scheduler = s; }

    // Assign a task to this employee (FIFO queue + Accepted bucket)
    void assignTask(const Task& t) {
        assignedTasks.push(t);
        statusBuckets[TaskStatus::ACCEPTED].push_back(t);
    }

    // Remove a task (used by undo of assignment)
    void removeTask(int taskId) {
        // Remove from FIFO queue
        std::queue<Task> fresh;
        while (!assignedTasks.empty()) {
            if (assignedTasks.front().id != taskId) {
                fresh.push(assignedTasks.front());
            }
            assignedTasks.pop();
        }
        // =============REVISION===============
        assignedTasks = std::move(fresh);

        // Remove from all status buckets
        for (auto& kv : statusBuckets) {
            auto& vec = kv.second;
            vec.erase(std::remove_if(vec.begin(), vec.end(),
                [taskId](const Task& t) { return t.id == taskId; }), vec.end());
        }
        //======================================
    }

    // Update task status - moves between O(1) buckets
    void updateTaskStatus(int taskId, TaskStatus newStatus) {
        // Find and remove from current bucket
        for (auto& kv : statusBuckets) {
            auto& vec = kv.second;
            auto it = std::remove_if(vec.begin(), vec.end(),
                [taskId](const Task& t) { return t.id == taskId; });
            if (it != vec.end()) {
                vec.erase(it, vec.end());
                break;
            }
        }
        // Update the task in the FIFO queue so viewMyTasks shows fresh status
        std::queue<Task> fresh;
        Task updated;
        bool found = false;
        while (!assignedTasks.empty()) {
            Task t = assignedTasks.front();
            assignedTasks.pop();
            if (t.id == taskId) {
                t.status = newStatus;
                updated = t;
                found = true;
            }
            fresh.push(t);
        }
        assignedTasks = std::move(fresh);

        // Add to new bucket
        if (found) {
            statusBuckets[newStatus].push_back(updated);
        }
    }

    // View tasks in FIFO order
    void viewMyTasks() const {
        std::queue<Task> temp = assignedTasks;
        if (temp.empty()) {
            std::cout << "  No tasks assigned yet.\n";
            return;
        }
        int count = 1;
        while (!temp.empty()) {
            Task t = temp.front();
            temp.pop();
            std::cout << "  " << count++ << ". [ID " << t.id << "] " << t.title
                      << " | Priority: " << priorityToString(t.priority)
                      << " | Status: " << statusToString(t.status) << "\n";
        }
    }

    // View status counts - O(1) bucket size
    void viewStatusCounts() const {
        std::cout << "  ┌─────────────────────────────────────┐\n";
        std::cout << "  │        TASK STATUS SUMMARY          │\n";
        std::cout << "  ├─────────────────────────────────────┤\n";
        std::cout << "  │ Accepted   : " << statusBuckets.at(TaskStatus::ACCEPTED).size() << "\n";
        std::cout << "  │ Working    : " << statusBuckets.at(TaskStatus::WORKING).size() << "\n";
        std::cout << "  │ Incomplete : " << statusBuckets.at(TaskStatus::INCOMPLETE).size() << "\n";
        std::cout << "  │ Failed     : " << statusBuckets.at(TaskStatus::FAILED).size() << "\n";
        std::cout << "  │ Completed  : " << statusBuckets.at(TaskStatus::COMPLETED).size() << "\n";
        std::cout << "  └─────────────────────────────────────┘\n";
    }

    // Check if a task belongs to this employee
    bool hasTask(int taskId) const {
        std::queue<Task> temp = assignedTasks;
        while (!temp.empty()) {
            if (temp.front().id == taskId) return true;
            temp.pop();
        }
        return false;
    }
};

#endif // EMPLOYEE_H