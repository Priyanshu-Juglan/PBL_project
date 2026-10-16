#ifndef TASKSCHEDULER_H
#define TASKSCHEDULER_H

#include <unordered_map>
#include <queue>
#include <stack>
#include <vector>
#include <string>
#include <iostream>
#include <memory>
#include <algorithm>

#include "Task.h"
#include "User.h"
#include "Employee.h"
#include "Admin.h"

// Action types for the undo stack
enum class ActionType { ADD_EMPLOYEE, REMOVE_EMPLOYEE, CREATE_TASK, ASSIGN_TASK };

// Encapsulated action record for LIFO undo
struct Action {
    ActionType type;
    int id;                 // employee id or task id
    std::string name;       // employee name or task title
    std::string password;   // for removed employee restore
    int taskId;             // for assign/remove
    int fromEmployee;       // for assign undo
    int toEmployee;         // for assign undo
    Priority priority;      // for task restore
    int deadline;           // for task restore
    std::string description;// for task restore
};

class TaskScheduler {
private:
    // Employee Registry - O(1) HashMap
    std::unordered_map<int, std::shared_ptr<Employee>> employees;

    // Master Task Record - central repository
    std::unordered_map<int, Task> masterTasks;

    // Unassigned Task Pool - priority queue (max-heap by urgency)
    std::priority_queue<Task> unassignedPool;

    // Undo History - LIFO stack
    std::stack<Action> undoStack;

    int nextEmployeeId;
    int nextTaskId;

    // Helper: rebuild the priority queue after undo (priority_queue has no clear)
    void rebuildUnassignedPool() {
        std::priority_queue<Task> fresh;
        for (auto& kv : masterTasks) {
            if (kv.second.assignedTo == -1) {
                fresh.push(kv.second);
            }
        }
        unassignedPool = std::move(fresh);
    }

public:
    TaskScheduler() : nextEmployeeId(1), nextTaskId(1) {}

    // ---- Authentication ----
    std::shared_ptr<User> authenticate(const std::string& uname, const std::string& pwd) {
        // Admin is a fixed account
        if (uname == "admin" && pwd == "admin123") {
            return std::make_shared<Admin>(0, "admin", "admin123");
        }
        for (auto& kv : employees) {
            if (kv.second->loginCheck(uname, pwd)) {
                return kv.second;
            }
        }
        return nullptr;
    }

    // ---- Admin: Add Employee (O(1)) ----
    bool addEmployee(const std::string& name, const std::string& pwd) {
        int id = nextEmployeeId++;
        employees[id] = std::make_shared<Employee>(id, name, pwd);
        undoStack.push({ActionType::ADD_EMPLOYEE, id, name, pwd, 0, -1, -1, Priority::LOW, 0, ""});
        return true;
    }

    // ---- Admin: Remove Employee ----
    bool removeEmployee(int id) {
        auto it = employees.find(id);
        if (it == employees.end()) return false;

        // Reallocate or flag associated tasks back to unassigned pool
        std::vector<int> toReassign;
        for (auto& kv : masterTasks) {
            if (kv.second.assignedTo == id) {
                kv.second.assignedTo = -1;
                kv.second.status = TaskStatus::ACCEPTED;
                toReassign.push_back(kv.first);
            }
        }
        for (int tid : toReassign) {
            unassignedPool.push(masterTasks[tid]);
        }

        Action a;
        a.type = ActionType::REMOVE_EMPLOYEE;
        a.id = id;
        a.name = it->second->getUsername();
        a.password = it->second->getPassword();
        a.taskId = -1;
        undoStack.push(a);

        employees.erase(it);
        return true;
    }

    // ---- Admin: Create Task (push to priority queue) ----
    bool createTask(const std::string& title, const std::string& desc, Priority prio, int deadline) {
        int id = nextTaskId++;
        Task t(id, title, desc, prio, deadline);
        masterTasks[id] = t;
        unassignedPool.push(t);
        undoStack.push({ActionType::CREATE_TASK, 0, title, "", id, -1, -1, prio, deadline, desc});
        return true;
    }

    // ---- Admin: Assign top-priority task to employee ----
    bool assignTask(int employeeId) {
        if (employees.find(employeeId) == employees.end()) return false;
        if (unassignedPool.empty()) return false;

        Task top = unassignedPool.top();
        unassignedPool.pop();

        top.assignedTo = employeeId;
        top.status = TaskStatus::ACCEPTED;
        masterTasks[top.id] = top;

        employees[employeeId]->assignTask(top);

        undoStack.push({ActionType::ASSIGN_TASK, 0, "", "", top.id, -1, employeeId, Priority::LOW, 0, ""});
        return true;
    }

    // ---- Admin: Undo last action ----
    bool undoLastAction() {
        if (undoStack.empty()) return false;
        Action a = undoStack.top();
        undoStack.pop();

        switch (a.type) {
            case ActionType::ADD_EMPLOYEE:
                // Remove the employee that was just added
                employees.erase(a.id);
                break;

            case ActionType::REMOVE_EMPLOYEE:
                // Restore the removed employee
                employees[a.id] = std::make_shared<Employee>(a.id, a.name, a.password);
                break;

            case ActionType::CREATE_TASK:
                // Remove the created task from master and pool
                masterTasks.erase(a.taskId);
                rebuildUnassignedPool();
                break;

            case ActionType::ASSIGN_TASK:
                // Unassign the task and return it to the pool
                if (masterTasks.find(a.taskId) != masterTasks.end()) {
                    masterTasks[a.taskId].assignedTo = -1;
                    masterTasks[a.taskId].status = TaskStatus::ACCEPTED;
                    employees[a.toEmployee]->removeTask(a.taskId);
                    unassignedPool.push(masterTasks[a.taskId]);
                }
                break;
        }
        return true;
    }

    // ---- Employee: Update task status (moves between O(1) buckets) ----
    bool updateTaskStatus(int employeeId, int taskId, TaskStatus newStatus) {
        auto it = employees.find(employeeId);
        if (it == employees.end()) return false;
        if (masterTasks.find(taskId) == masterTasks.end()) return false;
        if (masterTasks[taskId].assignedTo != employeeId) return false;

        masterTasks[taskId].status = newStatus;
        it->second->updateTaskStatus(taskId, newStatus);
        return true;
    }

    // ---- Employee: Accept / Start a task ----
    bool acceptTask(int employeeId, int taskId) {
        return updateTaskStatus(employeeId, taskId, TaskStatus::WORKING);
    }

    // ---- Getters for dashboards ----
    std::shared_ptr<Employee> getEmployee(int id) {
        auto it = employees.find(id);
        return (it != employees.end()) ? it->second : nullptr;
    }

    const std::unordered_map<int, std::shared_ptr<Employee>>& getEmployees() const {
        return employees;
    }

    const std::unordered_map<int, Task>& getMasterTasks() const {
        return masterTasks;
    }

    bool hasUnassignedTasks() const {
        return !unassignedPool.empty();
    }

    Task peekTopUnassigned() const {
        return unassignedPool.top();
    }

    int getNextEmployeeId() const { return nextEmployeeId; }
    int getNextTaskId() const { return nextTaskId; }
};

#endif // TASKSCHEDULER_H