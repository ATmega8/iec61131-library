//
// Created by life on 20-7-7.
//

#ifndef IEC61131_LIBRARY_GLUEVARS_H
#define IEC61131_LIBRARY_GLUEVARS_H
void glueVars();
void update_time();

uint8_t get_input_bool_value(int group, uint8_t bit);
uint8_t get_output_bool_value(int group, uint8_t bit);
void set_output_bool_value(int group, uint8_t bit, uint8_t value);

#endif //IEC61131_LIBRARY_GLUEVARS_H
