#include "dinamicStruct.h"


FastAccelStepperEngine engine = FastAccelStepperEngine();
FastAccelStepper *stepper[MAX_STEPPER];

Node* pbeg;
Node* pend;
Node* pav;

//чекаем на прохождение
void whileList() {
    pav = pbeg;
    while (pav) { // проверяем на действие
        if(pav->s){
            if(!(pav->s->isRunning())){
                Node* temp = pav;
                remove(&(temp->EngineNumber));
            }
        }
        pav = pav->next;
    }
}

//// 
// Добавление в конец списка
void add(uint8_t* d, int32_t  * MotorStep, uint32_t * MotorSpeed, int32_t * MotorAcceleration) {
    Serial.println();
    Serial.print("Add engine №");Serial.println((*d));
    Node* pv = new Node;
    pv->s = stepper[(*d)];
    if (pv->s){
        if(*MotorSpeed){
            (*MotorSpeed) *= 145;
            (*MotorSpeed) += 1000;
            (*MotorAcceleration) *= 145;
            (*MotorAcceleration) += 1000;
        }else{
            (*MotorSpeed) = 1000;
            (*MotorAcceleration) = 1000;
        }
        pv->s->setSpeedInHz((*MotorSpeed));
        pv->s->setAcceleration((*MotorAcceleration));
        pv->s->move(*MotorStep);
    }
    pv->MStep = (*MotorStep);
    pv->EngineNumber = (*d);
    if (!pbeg) {
        pv->next = 0; pv->prev = 0;
        pbeg = pv;
        pend = pbeg; // Список заканчивается, едва начавшись
        pav = pbeg;
    }
    else {
        pv->next = 0; pv->prev = pend;
        pend->next = pv;
        pend = pv;
    }
    Serial.println("Parameters");
    Serial.println("Steps: " + String(*MotorStep));
    Serial.println("Speed: " + String(*MotorSpeed));
    Serial.println("Acceleration: " + String(*MotorAcceleration));
}
//
// Поиск элемента по ключу
Node * find(uint8_t* d) {
    Node* pv = pbeg;
    while(pv){
        if(pv->EngineNumber == (*d))return pv;
        pv = pv->next;
    }
    return 0;
}
//
// Удаление элемента
bool remove(uint8_t* key) {
    if (Node *pkey = find(key)) {
        if (pkey == pbeg) { 
            if (pbeg->next != 0) {
                pbeg = pbeg->next;
                pbeg->prev = 0;
            }
        }
        else if (pkey == pend){
                if (pend->prev != 0) {
                    pend = pend->prev;
                    pend->next = 0;
                }
        }
    else {
        (pkey->prev)->next = pkey->next;
        (pkey->next)->prev = pkey->prev;
    }
        if (pkey->next == 0 && pkey->prev == 0) {
                pbeg = 0;
        }
        pkey->s->setCurrentPosition(0);
        pkey->s->setPositionAfterCommandsCompleted(0);
        pkey->s->stopMove();
        Serial.println("Removed from queue engine №" + String(pkey->EngineNumber));
        delete pkey;
        return true;
    }
    return false;
}