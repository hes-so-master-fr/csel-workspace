#include "../oled/ssd1306.h"

#define MODE_PATH "/dev/cooling/mode"
#define SPEED_PATH "/dev/cooling/speed"



static void set_screen(int temp, int freq, int duty){
    ssd1306_set_position (0,0);
    ssd1306_puts("Cooling ");
    ssd1306_set_position (0,1);
    ssd1306_puts("  Demo - SW");
    ssd1306_set_position (0,2);
    ssd1306_puts("--------------");

    ssd1306_set_position (0,3);
    ssd1306_puts("Temp: 35'C");
    ssd1306_set_position (0,4);
    ssd1306_puts("Freq: 1Hz");
    ssd1306_set_position (0,5);
    ssd1306_puts("Duty: 50%");

}


int main(){
    ssd1306_init();


    return 0;
}