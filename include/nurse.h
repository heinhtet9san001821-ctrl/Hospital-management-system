#ifndef HMS_NURSE_H
#define HMS_NURSE_H

#include "person.h"
#include <string>

namespace hms {

class Nurse : public Person {
public:
    // Constructor & Destructor //
    Nurse(const std::string& id,
          const std::string& name,
          int age,
          const std::string& gender,
          const std::string& phoneNumber,
          const std::string& nurseId,
          const std::string& assignedWard,
          const std::string& shiftTime);

   