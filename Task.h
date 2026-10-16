#ifndef TASK_H
#define TASK_H

#include <string>

// Task statuses tracked in O(1) buckets
enum class TaskStatus { ACCEPTED, WORKING, INCOMPLETE, FAILED, COMPLETED };

// Priority levels for the priority queue
enum class Priority { LOW, MEDIUM, HIGH };

// Convert status enum to readable string
inline std::string statusToString(TaskStatus s) {
    switch (s) {
        case TaskStatus::ACCEPTED:   return "Accepted";
        case TaskStatus::WORKING:    return "Working";
        case TaskStatus::INCOMPLETE: return "Incomplete";
        case TaskStatus::FAILED:     return "Failed";
        case TaskStatus::COMPLETED:  return "Completed";
    }
    return "Unknown";
}

// Convert priority enum to readable string
inline std::string priorityToString(Priority p) {
    switch (p) {
        case Priority::LOW:    return "Low";
        case Priority::MEDIUM: return "Medium";
        case Priority::HIGH:   return "High";
    }
    return "Unknown";
}

class Task {
public:
    int id;
    std::string title;
    std::string description;
    Priority priority;
    TaskStatus status;
    int assignedTo;   // employee id, -1 if unassigned
    int deadline;     // days until deadline

    Task()
        : id(0), title(""), description(""), priority(Priority::LOW),
          status(TaskStatus::ACCEPTED), assignedTo(-1), deadline(0) {}

    Task(int id, const std::string& title, const std::string& desc,
         Priority prio, int deadline)
        : id(id), title(title), description(desc), priority(prio),
          status(TaskStatus::ACCEPTED), assignedTo(-1), deadline(deadline) {}

    // Higher score = more urgent (used by priority_queue max-heap)
    int priorityScore() const {
        int p = 0;
        switch (priority) {
            case Priority::LOW:    p = 1; break;
            case Priority::MEDIUM: p = 2; break;
            case Priority::HIGH:   p = 3; break;
        }
        // Urgency: priority dominates, deadline breaks ties (sooner = higher)
        return p * 100 + (100 - deadline);
    }

    // For std::priority_queue (max-heap): "this" is lower priority than "other"
    // when its score is smaller, so the highest score sits on top.
    bool operator<(const Task& other) const {
        return priorityScore() < other.priorityScore();
    }
};

#endif // TASK_H