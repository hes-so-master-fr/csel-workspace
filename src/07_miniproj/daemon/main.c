#include <stdio.h>
#include "../oled/ssd1306.h"

#define MODE_PATH "/dev/cooling/mode"
#define SPEED_PATH "/dev/cooling/speed"

enum mode {manual, automatic};


static void set_screen(enum mode curr, int temp, int freq, int duty){
    ssd1306_clear_display();
    ssd1306_set_position (0,0);
    ssd1306_puts("-System Status-");
    ssd1306_set_position (0,1);
    ssd1306_puts("-Nanopi Cooling-");
    ssd1306_set_position (0,2);
    ssd1306_puts("--------------");
    char * curmode = (curr==manual) ? "manual" : "automatic";
    char mode_buf[50];
    sprintf(mode_buf, "mode: %s", curmode);
    ssd1306_set_position (0,3);
    ssd1306_puts(mode_buf);
    

    ssd1306_set_position (0,4);
    char temp_buf[50];
    sprintf(temp_buf, "Temp: %d 'C", temp);
    ssd1306_puts(temp_buf);
    ssd1306_set_position (0,5);
    char freq_buf[50];
    sprintf(freq_buf, "Freq: %dHz", freq);
    ssd1306_puts(freq_buf);
    ssd1306_set_position (0,6);
    char duty_buf[50];
    sprintf(duty_buf, "Duty %d%%", duty);
    ssd1306_puts(duty_buf);

}


int main(){
    ssd1306_init();
    set_screen(automatic,3,3,3);

    return 0;
}