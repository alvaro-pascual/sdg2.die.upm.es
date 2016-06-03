#include <stdlib.h>  // para el NULL
#include <stdio.h>
#include <wiringPi.h>
#include "fsm.h"
#include "tmr.h"


#define CLK_MS 10
#define TIMEOUT_JUGADOR 3000
#define TIMEOUT_C 5000

#define ALIMENTACION 07

#define GPIO_PULSADOR_1 19
#define GPIO_PULSADOR_2 20
#define GPIO_PULSADOR_3 21
#define GPIO_PULSADOR_4 26
#define GPIO_PULSADOR_START 27


#define GPIO_LED_1 00
#define GPIO_LED_2 01
#define GPIO_LED_3 02
#define GPIO_LED_4 03
#define GPIO_LED_START 04
#define GPIO_BUFFER 8

#define FLAG_PULSADOR_1	0x01
#define FLAG_PULSADOR_2	0x02
#define FLAG_PULSADOR_3	0x04
#define FLAG_PULSADOR_4 0x08
#define FLAG_PULSADOR_START 0x10
#define FLAG_TIMER 0x20

#define MASCARA 0x2F
int NUM_LEDS_ENCENDIDOS = 15;
#define PENALTY_TIME 5000
#define NUM_FALLOS 3

#define ANTIRREBOTE 150
#define T_D3 500

enum fsm_state {
	WAIT_START,
	WAIT_PUSH,
	WAIT_END,
	EXCTN_WAIT_END
};

int led_encendido=0;
int led=0;
int contador = 0;
int t_juego = TIMEOUT_JUGADOR;



void apagar() {
	digitalWrite(GPIO_LED_1, 0);
	digitalWrite(GPIO_LED_2, 0);//Apaga luces
	digitalWrite(GPIO_LED_3, 0);
	digitalWrite(GPIO_LED_4, 0);
	digitalWrite(GPIO_LED_START, 1);
}
void encender () {
	digitalWrite(GPIO_LED_1, 1);
	digitalWrite(GPIO_LED_2, 1);//Enciende luces
	digitalWrite(GPIO_LED_3, 1);
	digitalWrite(GPIO_LED_4, 1);
	digitalWrite(GPIO_LED_START, 0);
}

void sonido_fail(){
	int k;
	for(k = 0; k < 50; k++){
		digitalWrite(GPIO_BUFFER, 1);
		delay(5);
		digitalWrite(GPIO_BUFFER, 0);
		delay(5);
	}
}
void sonido_ok(){
	int k;
	for(k = 0; k < 13; k++){
		digitalWrite(GPIO_BUFFER, 1);
		delay(20);
		digitalWrite(GPIO_BUFFER, 0);
		delay(20);
	}
}
void sonido_final(){
	int k;
	for(k = 0; k < 8; k++){
		digitalWrite(GPIO_BUFFER, 1);
		delay(40);
		digitalWrite(GPIO_BUFFER, 0);
		delay(40);
	}
}


int encender_led_random(){

	//inserte codigo aqui, asignar led_encendido como si fuese un flag similar a pulsador
	int numero;

	numero = rand() % 4 + 1;
	if(numero<1){
		led_encendido|= FLAG_PULSADOR_1;
		led=GPIO_LED_1; //GPIO_LED_1
	}
	else if(numero<2){
		led_encendido|= FLAG_PULSADOR_2;
		led=GPIO_LED_2; //GPIO_LED_2
	}
	else if(numero<3){
		led_encendido|= FLAG_PULSADOR_3;
		led=GPIO_LED_3; //GPIO_LED_3
	}
	else{
		led_encendido|= FLAG_PULSADOR_4;
		led=GPIO_LED_4; //GPIO_LED_4
	}
	return led;

}


volatile int flags = 0;
int led_juego=0;
int inicio = 0;
int fin = 0;
int tiempos[30];
int fallos=0;
int dificultad = 0;
int tiempo3 = 0;

void boton_p1_isr (void) {
	static int last = 0;
	int now = millis();
	if (now - last > ANTIRREBOTE){
		last = now;
		flags = 0;
		flags|= FLAG_PULSADOR_1;
			fin=millis();
	}
}



void boton_p2_isr (void) {
	static int last = 0;
	int now = millis();
	if (now - last > ANTIRREBOTE){
		last = now;
		flags = 0;
		flags|= FLAG_PULSADOR_2;
		fin=millis();
	}
}
void boton_p3_isr (void) {
	static int last = 0;
	int now = millis();
	if (now - last > ANTIRREBOTE){
		last = now;
		flags = 0;
		flags|= FLAG_PULSADOR_3;
		fin=millis();
	}
}
void boton_p4_isr (void) {
	static int last = 0;
	int now = millis();
	if (now - last > ANTIRREBOTE){
		last = now;
		flags = 0;
		flags|= FLAG_PULSADOR_4;
		fin=millis();
	}
}
void boton_pStart_isr (void) {
	static int last = 0;
	int now = millis();
	if (now - last > ANTIRREBOTE){
		last = now;
		flags = 0;
		flags|= FLAG_PULSADOR_START;

		}
}
void timer_isr (union sigval value) {flags|= FLAG_TIMER; }

//funcion que comprueba si hemos fallado al pulsar el boton
int comp_fail(int led1){
	if(dificultad == 3){
		int now = millis();
		if (now - tiempo3 > T_D3){
		digitalWrite(led_juego, 0);
		}
	}

	led1= led1 & MASCARA;
	flags = flags & MASCARA;
	if(flags > 0){
	return ~(led1& flags);
	}
	else return 0;
}

//funcion que comprueba si hemos acertado al pulsar el boton
int comp_ok(int led2){
	led2= led2 & MASCARA;
	flags = flags & MASCARA;
	return (flags & led2);
}

int comp_exception(){

	int suma = 0;
	if(flags & FLAG_PULSADOR_1){
		suma++;
	}
	if(flags & FLAG_PULSADOR_2){
			suma++;
		}
	if(flags & FLAG_PULSADOR_3){
			suma++;
		}
	if(flags & FLAG_PULSADOR_4){
			suma++;
		}

	if (suma > 1){
		return 1;
	}
	else {return 0;}
}

int event_btn_start_end (fsm_t* this) { //
	if (flags& FLAG_PULSADOR_START){
		return 1;
	}else {
		return 0;
	}
	//return (flags& FLAG_PULSADOR_START); }
}
int event_btn_fail (fsm_t* this) { return ((flags& FLAG_TIMER) || comp_fail(led_encendido)); }
int event_btn_ok (fsm_t* this) { return (comp_ok(led_encendido)); }
int event_exception (fsm_t* this) { return (comp_exception()); }
int event_end_game (fsm_t* this) { return (contador==NUM_LEDS_ENCENDIDOS)||(fallos==NUM_FALLOS); }
int timeout (fsm_t* this) { return (flags& FLAG_TIMER); }
int t_led (fsm_t* this) {
	int now = millis();
	return (now - tiempo3 > T_D3);
}







/*
int pulsado_p (fsm_t* this) { return (flags& FLAG_BOTON_P); }
int pulsado_c1 (fsm_t* this) { return (flags& FLAG_BOTON_C1); }
int pulsado_c2 (fsm_t* this) { return (flags& FLAG_BOTON_C2); }
int timeout (fsm_t* this) { return (flags& FLAG_TIMER); }

void enciende_p (fsm_t* this) {
	flags &= (~FLAG_BOTON_P);//Limpia flag
	digitalWrite(GPIO_LIGHT_P, 1);//Enciende luz
	tmr_startms((tmr_t*)(this->user_data), TIMEOUT_P);//Lanza cuenta
}
void enciende_l1_1 (fsm_t* this) {
	flags &= (~FLAG_BOTON_C1);//Limpia flag
	digitalWrite(GPIO_LIGHT_L1_1, 1);//Enciende luz
	tmr_startms((tmr_t*)(this->user_data), TIMEOUT_C);//Lanza cuenta
}
void enciende_l1_2 (fsm_t* this) {
	flags &= (~FLAG_BOTON_C1);//Limpia flag
	digitalWrite(GPIO_LIGHT_L1_2, 1);//Enciende luz
	tmr_startms((tmr_t*)(this->user_data), TIMEOUT_C);
}
void enciende_l2_1 (fsm_t* this) {
	flags &= (~FLAG_BOTON_C2);
	digitalWrite(GPIO_LIGHT_L2_1, 1);
	tmr_startms((tmr_t*)(this->user_data), TIMEOUT_C);
}
void enciende_l2_2 (fsm_t* this) {
	flags &= (~FLAG_BOTON_C2);//
	digitalWrite(GPIO_LIGHT_L2_2, 1);
	tmr_startms((tmr_t*)(this->user_data), TIMEOUT_C);
}
*/

void elegirDif(){
	printf("Elegir dificultad 1, 2 o 3.\n");
	int y = 1;
	while(y){
	scanf("%d", &dificultad);
	if(dificultad == 1){
		NUM_LEDS_ENCENDIDOS = 15;
		y=0;
	}
	if(dificultad == 2){
			NUM_LEDS_ENCENDIDOS = 30;
			y=0;
		}
	if(dificultad == 3){
			NUM_LEDS_ENCENDIDOS = 15;
			y=0;
		}
	}
}


void s1e1(fsm_t* this){
	flags =0; //Limpia flags
	elegirDif();

	digitalWrite(GPIO_LED_START, 0); //Apaga led start
	led_juego = encender_led_random();
	digitalWrite(led_juego, 1); //enciende led de juego
	inicio=millis();
	contador ++;
	tiempo3 = millis();
	tmr_startms((tmr_t*)(this->user_data), t_juego);
	printf("pasa de 1 al 2\n");

}


void s2e2(fsm_t* this){
	tiempos[contador - 1] = (fin - inicio);
	sonido_ok();
	/*switch(led_juego)
	{
	case 11:
		flags &= (~FLAG_PULSADOR_1); //Limpia flags
	    break;
	case 14:
		flags &= (~FLAG_PULSADOR_2); //Limpia flags
	    break;
	case 17:
		flags &= (~FLAG_PULSADOR_3); //Limpia flags
	    break;
	case 18:
		flags &= (~FLAG_PULSADOR_4); //Limpia flags
		break;
	default:*/
	    flags=0;

	digitalWrite(led_juego, 0); //Apaga led start
	led_encendido = 0;
	led_juego = encender_led_random();
	digitalWrite(led_juego, 1); //enciende led de juego
	inicio=millis();
	contador ++;
	t_juego = t_juego - 100;
	tiempo3 = millis();
	tmr_startms((tmr_t*)(this->user_data), t_juego);
	printf("pasa de 2 al 2; ok\n");

}
void s2e3(fsm_t* this){
	tiempos[contador - 1] = PENALTY_TIME;
	fallos++;
	sonido_fail();
	/*switch(led_juego)
	{
	case 11:
		flags &= (~FLAG_PULSADOR_1); //Limpia flags
	    break;
	case 14:
		flags &= (~FLAG_PULSADOR_2); //Limpia flags
	    break;
	case 17:
		flags &= (~FLAG_PULSADOR_3); //Limpia flags
	    break;
	case 18:
		flags &= (~FLAG_PULSADOR_4); //Limpia flags
		break;
	default:*/
	    flags=0;

	digitalWrite(led_juego, 0); //Apaga led start
	led_encendido = 0;
	led_juego = encender_led_random();
	digitalWrite(led_juego, 1); //enciende led de juego
	inicio=millis();
	contador ++;
	t_juego = t_juego - 100;
	tiempo3 = millis();
	tmr_startms((tmr_t*)(this->user_data), t_juego);
	printf("pasa de 2 al 2; fallo\n");
}
void s2e4(fsm_t* this){
	if (fallos != 3){
	tiempos[contador - 1] = (fin - inicio);
	}
	sonido_final();

	/*switch(led_juego)
	{
	case 11:
		flags &= (~FLAG_PULSADOR_1); //Limpia flags
	    break;
	case 14:
		flags &= (~FLAG_PULSADOR_2); //Limpia flags
	    break;
	case 17:
		flags &= (~FLAG_PULSADOR_3); //Limpia flags
	    break;
	case 18:
		flags &= (~FLAG_PULSADOR_4); //Limpia flags
		break;
	default:*/
	    flags=0;

	digitalWrite(led_juego, 0); //Apaga led start
	led_encendido = 0;
	encender();
	printf("pasa de 2 al 3\n");


}
void s2e5(fsm_t* this){

	sonido_final();
	digitalWrite(led_juego, 0); //Apaga led start
	led_encendido = 0;
	encender();




	printf("pasa de 2 al 4\n");
}
void s3e1(fsm_t* this){

	apagar();


	int i;
	printf("Los tiempos son:\n ");
	int min = tiempos[0];
	int max = tiempos[0];
	int total = 0;
	for(i = 0;i<contador-1;i++){
		if(min > tiempos[i]){
			min = tiempos[i];
		}
		if(max < tiempos[i]){
			max = tiempos[i];
		}
		total += tiempos[i];
		printf("%i tiempo %d\n", i, tiempos[i]);
	}
	total = total/contador;
	printf("El tiempo m\EDnimo ha sido de %d ms\n", min);
	printf("El tiempo m\E1ximo ha sido de %d ms\n", max);
	printf("El tiempo medio ha sido de %d ms\n", total);
	printf("El n\FAmero de fallos ha sido: %d \n", fallos);


	printf("pasa de 3|4 al 5\n");

	fallos = 0;
	led_encendido=0;
		led=0;
		contador = 0;
		flags = 0;
		led_juego=0;
		inicio = 0;
		fin = 0;
		t_juego = TIMEOUT_JUGADOR;
		dificultad=0;

		int j;
		for(j=0; j<30;j++){
			tiempos[j]=0;
		}

}




// wait until next_activation (absolute time)
void delay_until (unsigned int next)
{
  unsigned int now = millis();
  if (next > now) {
	  delay (next - now);
  }
}

int main ()
{
	tmr_t* tmr = tmr_new (timer_isr);//user_data: instancia del timer

	fsm_trans_t tv_tt[] = {
			{WAIT_START, event_btn_start_end, WAIT_PUSH, s1e1},
			{WAIT_PUSH, event_exception, EXCTN_WAIT_END, s2e5},
			{WAIT_PUSH, event_end_game, WAIT_END, s2e4},

			{WAIT_PUSH, event_btn_ok, WAIT_PUSH, s2e2},
			{WAIT_PUSH, event_btn_fail, WAIT_PUSH, s2e3},
			{EXCTN_WAIT_END, event_btn_start_end, WAIT_START, s3e1},
			{WAIT_END, event_btn_start_end, WAIT_START, s3e1},

			/*

			{IDLE, pulsado_c1, WAIT_LED, enciende_l1_2},
			{IDLE, pulsado_c2, WAIT_LED, enciende_l2_2},
			{WAIT_BUT, timeout,IDLE, apagar},
			{WAIT_BUT, pulsado_c1,WAIT_LED, enciende_l1_1},
			{WAIT_BUT, pulsado_c2,WAIT_LED, enciende_l2_1},
			{WAIT_LED, timeout,IDLE, apagar},*/
			{-1, NULL, -1, NULL}
	};

	fsm_t* tv_fsm = fsm_new (WAIT_START, tv_tt, tmr);
	unsigned int next;

	wiringPiSetupGpio();
	pinMode(GPIO_PULSADOR_1, INPUT);
	pinMode(GPIO_PULSADOR_2, INPUT);
	pinMode(GPIO_PULSADOR_3, INPUT);
	pinMode(GPIO_PULSADOR_4, INPUT);
	pinMode(GPIO_PULSADOR_START, INPUT);
	pinMode(GPIO_LED_1, OUTPUT);
	pinMode(GPIO_LED_2, OUTPUT);
	pinMode(GPIO_LED_3, OUTPUT);
	pinMode(GPIO_LED_4, OUTPUT);
	pinMode(GPIO_LED_START, OUTPUT);
	pinMode(ALIMENTACION, OUTPUT);
	pinMode(GPIO_BUFFER, OUTPUT);

	digitalWrite(ALIMENTACION, 1);
	digitalWrite(GPIO_BUFFER, 0);

	apagar();

	wiringPiISR(GPIO_PULSADOR_1, INT_EDGE_FALLING, boton_p1_isr);
	wiringPiISR(GPIO_PULSADOR_2, INT_EDGE_FALLING, boton_p2_isr);
	wiringPiISR(GPIO_PULSADOR_3, INT_EDGE_FALLING, boton_p3_isr);
	wiringPiISR(GPIO_PULSADOR_4, INT_EDGE_FALLING, boton_p4_isr);
	wiringPiISR(GPIO_PULSADOR_START, INT_EDGE_FALLING, boton_pStart_isr);



	//Configuraci\F3n de GPIO: modo, interrupciones
	//Ejecutar funci\F3n para inicializar los recursos: salidas, timer, flags, ...

	next = millis();
	while (1) {
		fsm_fire (tv_fsm);
		next += CLK_MS;
		delay_until (next);
	}

	tmr_destroy((tmr_t*)tv_fsm->user_data);//Destruir user_data
	fsm_destroy(tv_fsm);//Destruir maquina de estados
}
