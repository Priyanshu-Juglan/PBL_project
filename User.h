#ifndef USER_H
#define USER_H

#include <string>

// Abstract base class - Abstraction principle
class User {
protected:
    int id;
    std::string username;
    std::string password;

public:
    User(int id, const std::string& username, const std::string& password)
        : id(id), username(username), password(password) {}

    virtual ~User() {}

    // Pure virtual functions make User abstract
    virtual void showDashboard() = 0;
    virtual std::string getRole() const = 0;

    // Shared behavior - Inheritance principle
    bool loginCheck(const std::string& uname, const std::string& pwd) const {
        return username == uname && password == pwd;
    }

    int getId() const { return id; }
    std::string getUsername() const { return username; }
};

#endif // USER_H