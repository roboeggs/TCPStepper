#ifndef DINAMICSTRICT_H_
#define DINAMICSTRICT_H_
#include <map>
#include "FastAccelStepper.h"

struct Node
{
    FastAccelStepper *s = NULL;
    uint8_t EngineNumber;
    uint32_t MStep;
    Node *next;
    Node* prev;
};
void whileList();
Node * first(uint8_t* d, uint32_t * MotorStep);
void add(uint8_t* d, int32_t  * MotorStep, uint32_t * MotorSpeed, int32_t * MotorAcceleration);
Node * find(uint8_t* d);
bool remove(uint8_t* key);
#endif