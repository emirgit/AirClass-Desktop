#ifndef TESTDATA_H
#define TESTDATA_H

#include <QStringList>
#include <QMap>

class TestData {
public:
    static QStringList getTestStudents() {
        return {
            "John Doe (Present)",
            "Jane Smith (Present)",
            "Bob Johnson (Late)",
            "Alice Brown (Absent)",
            "Charlie Wilson (Present)"
        };
    }

    static QStringList getTestAttendanceData() {
        return {
            "Student ID,Name,Status,Time",
            "S001,John Doe,Present,2024-03-20 09:00:00",
            "S002,Jane Smith,Present,2024-03-20 09:01:15",
            "S003,Bob Johnson,Late,2024-03-20 09:15:30",
            "S004,Alice Brown,Absent,2024-03-20 09:00:00",
            "S005,Charlie Wilson,Present,2024-03-20 08:55:00"
        };
    }

    static QMap<QString, QString> getTestStudentDetails() {
        QMap<QString, QString> details;
        details["S001"] = "John Doe - Computer Science - Year 3";
        details["S002"] = "Jane Smith - Mathematics - Year 2";
        details["S003"] = "Bob Johnson - Physics - Year 4";
        details["S004"] = "Alice Brown - Chemistry - Year 1";
        details["S005"] = "Charlie Wilson - Biology - Year 3";
        return details;
    }

    static QStringList getTestSpeakRequests() {
        return {
            "S001,John Doe,I have a question about the last topic",
            "S003,Bob Johnson,Can you explain the formula again?",
            "S005,Charlie Wilson,I need clarification on the assignment"
        };
    }
};

#endif // TESTDATA_H 