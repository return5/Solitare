/*----------------------------------------- program info------------------------------------------*/
	/* simple solitare game. written in c and uses ncurses to display to terminal.
	 * written by: github/return5
	 * licensed under GPL v.2
	 */

/*------------------------------------------------------------------------------------------------*/

#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

/*-------------------------------------------- enums,typedefs and macros -------------------------*/ 

enum Suites{Hearts=1,Diamonds=1,Clubs=2,Spades=2,Dummy=9};
enum Showing{YES,NO};

typedef struct Card{
	int value,column_index,stack_index;
	char face[3];  //face of card
	enum Suites suite;  
	enum Showing show;  //if card is face up or face down.
	struct Card *next;
}Card;

typedef struct Column{
	Card *stack[24];
	int bottom;  //index of card on bottom of stack
	WINDOW *column_win; //window to display cards on
}Column;

/*-------------------------------------------- prototypes  ---------------------------------------*/
void printFace(Card *const card, const int y_offset,const int x_offset,WINDOW *const col_win);
void printCard(Card *const card,const int y_offset,const int x_offset,WINDOW *const col_win);
void printColumn(Column *const column);
void printAllColumns(void);
int checkSuite(Card *const c1, Card *const c2);
void removeCardFromDrawPile(void);
void moveFromDrawPile(Column *const to,const int i,const int col_index);
void moveCard(Column *const from, Column *const to, const int a, const int col_index);
void checkColumnOfCards(Column *const from,const int y, int *const card_selected);
void findMoveToDiscard(void); 
void nextDrawCard(void);
int getXIndex(const int x);
void setSelected(const int x, const int y, int *const card_selected);
void selectCard(const int x, const int y,int *const card_selected);
void gameLoop(void);
void gameWon(void); 
void newGame(void); 
void resetUsed(void); 
void deleteWin(void); 
void freeMemory(void); 
char *getFace(const int value);
int checkIfUsed(const int index);
int getCardIndex(void);
void initDrawPile(void);
void initDeck(void);
void initColumns(void);
void initDiscardPiles(void);
void initColors(void);
void initScreen(void);
void initGame(void);

/*-------------------------------------------- globals vars --------------------------------------*/
static Card *deck[52];  //entire deck
static Column *columns[7]; //columns cards wil be stored on
static Column *discard[4]; //discard piles
static Card *draw_pile;  //linked list holds draw pile
static Card *current_card;  //holds card currently face up in draw pile
static Card *Selected; //card which user currently has selected
static int used[52]; //array to hold index values. used when randomly putting cards onto columns
static int used_index = 0; //index to above array
static WINDOW *deck_win; //window for displaying deck of cards to choose from
static const size_t SIZE_CARD = sizeof(Card); //size of Card struct
static const size_t SIZE_COLUMN = sizeof(Column); //size of column struct
static int play_game = 1;  //controls gameLoop. 1 = play, 0 = stop

/*-------------------------------------------- code  ---------------------------------------------*/


void printFace(Card *const card, const int y_offset,const int x_offset,WINDOW *const col_win) {
	wattron(col_win,COLOR_PAIR(card->suite)|A_BOLD);
	mvwprintw(col_win,y_offset+1,x_offset + 1,"%-2s",card->face); //upper left corner
	mvwprintw(col_win,y_offset+1,x_offset + 6,"%2s",card->face); //upper right corner
	mvwprintw(col_win,y_offset+3,x_offset + 4,"%-2s",card->face); //middle
	mvwprintw(col_win,y_offset+5,x_offset + 1,"%-2s",card->face); //lower left corner
	mvwprintw(col_win,y_offset+5,x_offset + 6,"%2s",card->face); //lower right corner
	wattroff(col_win,COLOR_PAIR(card->suite)|A_BOLD);
}

void printCard(Card *const card,const int y_offset,const int x_offset,WINDOW *const col_win) {
	mvwprintw(col_win,y_offset,x_offset,"---------"); //top of card
	mvwprintw(col_win,y_offset+6,x_offset,"---------"); //bottom of card
	for(int i = 1; i < 6; i++) {
		mvwprintw(col_win,y_offset+i,x_offset,"|       |");//body of card
	}
	if(card->show == YES) {
		printFace(card,y_offset,x_offset,col_win);
	}
}

void printColumn(Column *const column) {
	wclear(column->column_win);
	for(int i = 0; i <= column->bottom;i++) {
		printCard(column->stack[i],i*4,0,column->column_win);
	}
	wrefresh(column->column_win);
}

void printAllColumns(void) {
	for(int i = 0; i < 7; i++) {
		printColumn(columns[i]);
	}
}

void checkIfWin(void) {
	for(int i = 0; i < 7; i++) {
		if(columns[i]->bottom >= 0) {
			return;
		}
	}
	gameWon();
}

int checkSuite(Card *const c1, Card *const c2) {
	if(c1->suite == c2->suite || c1->suite ==  Dummy ){
		return 1;
	}
	return -1;
}

void removeCardFromDrawPile(void) {
	Card *temp = draw_pile;
	if(current_card == draw_pile ) {
		if(current_card->next != NULL) {
			temp = draw_pile = current_card->next;
		}
		else { //all cards have been used, so set drawpile to nonsense card
			temp = malloc(SIZE_CARD);
			temp->value = 100;
			temp->show = NO;
			temp->next = NULL;
			draw_pile = temp;
			wclear(deck_win);
			wrefresh(deck_win);
		}
	}
	else { 
		while(temp->next != current_card  && temp->next != NULL) {
			temp = temp->next;
		}
		temp->next = temp->next->next;
	}
	if(temp->value != 100) {
		current_card = temp;	
		printCard(current_card,0,11,deck_win);
		wrefresh(deck_win);
	}
}

void moveFromDrawPile(Column *const to,const int i,const int col_index) {
	const int bottom = to->bottom;
	if((bottom < 0 && Selected->value == 13) || (bottom >= 0 && Selected->value == to->stack[bottom]->value + i && checkSuite(to->stack[bottom],Selected) == i)) {
		Selected->column_index = col_index;
		Selected->stack_index = ++(to->bottom);
		to->stack[to->bottom] = Selected;
		removeCardFromDrawPile();
		(i == -1) ? printColumn(to) : printCard(to->stack[to->bottom],1,2,to->column_win); //if i = 1, print discard pile, otherwise print regular column
		wrefresh(to->column_win);
		checkIfWin();
	}
}

void moveCard(Column *const from, Column *const to, const int a, const int col_index) {
	const int bottom = to->bottom;
	//if column is empty, then makes sure moving a king. otherwise make sure card is correct value and suite
	if((bottom < 0 && Selected->value == 13) || (bottom >= 0 && Selected->value == to->stack[bottom]->value + a && checkSuite(to->stack[bottom],Selected) == a )) {
		const int limit = from->bottom;
		for(int i = Selected->stack_index; i <= limit; i++) {
			(to->bottom)++;
			to->stack[to->bottom] = from->stack[i];
			to->stack[to->bottom]->stack_index = to->bottom;
			to->stack[to->bottom]->column_index = col_index;
			(from->bottom)--;
		}
		(a == -1) ? printColumn(to) : printCard(to->stack[to->bottom],1,2,to->column_win); //if a = 1, print discard pile, otherwise print regular column
		printColumn(from);
		wrefresh(to->column_win);
		checkIfWin();
	}
}	

//if user selects card not at bottom, check to see if all card below it are in order
void checkColumnOfCards(Column *const from,const int y, int *const card_selected){
	for(int i = y ; i < from->bottom;i++) {
		//if card below is not 1 lower in value and/or is not the opposite suite 
		if(from->stack[i]->value != from->stack[i+1]->value+1 && (checkSuite(from->stack[i],from->stack[i+1]) == 1)) {
			*card_selected = 0;
			return;
		}
	}
	Selected = from->stack[y];
	*card_selected = 1;	
}

//if user double clicks a card, check to see if it can be moved to discard pile. if it can, then do it. 
void findMoveToDiscard(void) {
	//if card is top of draw pile or is the bottom of its column and isnt an ace
	if((Selected == current_card || Selected->stack_index == columns[Selected->column_index]->bottom) && Selected->value != 1) {
		for(int i = 0; i < 4; i++) {
			//selected card is one less than card on discard[i] and the suite matches
			if(discard[i]->stack[discard[i]->bottom]->value == Selected->value - 1 && checkSuite(discard[i]->stack[discard[i]->bottom],Selected) == 1) {
				//is selected card comes from draw pile or is it from a column
				(Selected == current_card) ? moveFromDrawPile(discard[i],1,i) : moveCard(columns[Selected->column_index],discard[i],1,i);
				return;
			}
		}
	}
	//if selected is ace
	else if(Selected->value == 1) {
		for(int i = 0; i < 4; i++) {
			//discard[i] is empty
			if(discard[i]->bottom == 0 ) {
				(Selected == current_card) ? moveFromDrawPile(discard[i],1,i) : moveCard(columns[Selected->column_index],discard[i],1,i);
				return;
			}
		}
	}
}

//user clicks draw pile, get the next card in the pile
void nextDrawCard(void) {
	//if current card is last in pile, set the next card to first card in pile. otherwise set current_card to next card in pile
	current_card = (current_card->next == NULL) ? draw_pile : current_card->next;
	current_card->show=YES;	//show face value
	printCard(current_card,0,11,deck_win);
	wrefresh(deck_win);	
}

//takes x coordinate of where user clicked and converts that into the index for either columns array or discard_pile array
int getXIndex(const int x) {
	switch(x) {
		case 0 ... 10: return 0;//column of card
		case 11 ... 21: return 1;
		case 22 ... 32: return 2;
		case 33 ... 43: return 3;
		case 44 ... 54: return 4;
		case 55 ... 65: return 5;
		case 66 ... 76: return 6;
		case 80 ... 93: return 0;//discard piles
		case 94 ... 107: return 1;
		case 108 ... 121: return 2;
		case 122 ... 135: return 3;	
		case 136 ... 149: return 4;
		default: return 0; //didnt select either card nor discard piles defaults to 0
	}
}

//sets selected card to what user clicks on, provided it is a valid choice
void setSelected(const int x, const int y, int *const card_selected) {
	int const x_index = getXIndex(x); 	//get x index form x cordinates of where user clicked
	const int bottom = columns[x_index]->bottom;	 //index for bottom of stack
	if(x < 80 && bottom >= 0) { 	//user clicks on column of cards and it isnt empty
		const int bottom_offset = columns[x_index]->bottom * 4;	 //top of card on bottom of column
		if(y >= bottom_offset && y < bottom_offset + 7) { 	//if user clicks on card at bottom of column
			if(columns[x_index]->stack[bottom]->show == NO) {	 //if bottom card isnt showing face
				columns[x_index]->stack[bottom]->show = YES; 	//flip it over to show face
				printColumn(columns[x_index]);
			}
			else { 	//otherwise set Slected to bottom card
			Selected = columns[x_index]->stack[bottom];
			*card_selected = 1;
			}
		}
		else if (y  <= bottom_offset) { 	//if user clicks on card not on bottom of stack
			checkColumnOfCards(columns[x_index],(y-1)/4,card_selected);
		}
	}
	else if(y > 13 && y < 24) {	
		if(x > 93 && x < 103) { 	//user clicks on draw pile
			nextDrawCard();
		}
		else if (x > 104 && x < 114) { 	//user clicks on draw card
			Selected = current_card;
			*card_selected = 1;
		}
	}
}

//if user clicks on card, select it and store it in Selected
void selectCard(const int x, const int y,int *const card_selected) {
	if(*card_selected == 0 ) { //if user hasnt already selected a card
		setSelected(x,y,card_selected);
	}
	else { //if user already has selected card, then try to move selected card to where user then selects
		const int x_index = getXIndex(x);
		if(x < 80) { //user clicked on column of cards
			if (Selected == current_card) { //if selected card is in draw pile
			   	moveFromDrawPile(columns[x_index],-1,x_index);
			}
			else { 
				moveCard(columns[Selected->column_index],columns[x_index],-1,x_index);
			}
		}
		else if (x > 80 && x < 149 && y < 10) { //use clicked on discard pile
			if (Selected == current_card) {
			   	moveFromDrawPile(discard[x_index],1,x_index);
			}
			//only move to discard pile if Selected is bottom of column
			else if(Selected == columns[Selected->column_index]->stack[columns[Selected->column_index]->bottom]) { 
				moveCard(columns[Selected->column_index],discard[x_index],1,x_index);
			}
		}
		*card_selected = 0; //user no longer has a card selected
	}
}

//game loop. runs loop until user presses q
void gameLoop(void) {
	int card_selected = 0; //if user has selected a card or not. 0 = no, 1 = yes
	MEVENT event; //mouse event struct
	while(play_game == 1) {
		switch(getch()) {
			case 'q' : play_game = 0;
				break;
			case KEY_MOUSE:
				if(getmouse(&event) == OK) { 
					if(event.bstate & BUTTON1_CLICKED){ //if left mouse button was clicked
						 selectCard(event.x,event.y,&card_selected);
					}
					else if(event.bstate & BUTTON1_DOUBLE_CLICKED){ //if left mouse button was double clicked
						card_selected = 0;
						selectCard(event.x,event.y,&card_selected);
						if(card_selected == 1) {
							findMoveToDiscard();
						}
						card_selected = 0;
					}
				}
			default://do nothing
				break;
		}
	}
}

void gameWon(void) {
	mvprintw(5,5,"congragulations, you won! press q to quit, otherwise press any other key to play new game.");
	play_game = 0;
	switch(getch()) {
	case 'q': //do nothing. ends game
		break;
	default : newGame();
		break;
	}
}

//if user wants a new game, create it.
void newGame(void) {
	deleteWin();
	freeMemory();
	resetUsed();
	initGame();
	play_game = 1;
	gameLoop();
}


void resetUsed(void) {
	used_index = 0;
	for(int i = 0; i < 52; i++) {
		used[i] = -1;
	}
}

//free up memeory allocated by malloc before starting new game.
void freeMemory(void) {
	for(int i = 0; i < 52; i++) {
		free(deck[i]);
	}
	for(int i = 0; i < 7; i++) {
		free(columns[i]);	
	}
	for (int i = 0; i < 4; i++) {
		free(discard[i]->stack[0]);
		free(discard[i]);
	}	

	draw_pile = NULL;
}

void deleteWin(void) {
	for(int i = 0; i < 7; i++) {
		wclear(columns[i]->column_win);
		delwin(columns[i]->column_win);
	}
	for(int i = 0; i < 4; i++) {
		wclear(discard[i]->column_win);
		delwin(discard[i]->column_win);
	}
	wclear(deck_win);
	delwin(deck_win);
	clear();
	refresh();
}

//returns face value as a char pointer
char *getFace(const int value){
	char *c = malloc(3);
	switch(value) {
		case 1:	return "A";
		case 2 ... 10:
			snprintf(c,3,"%d",value);
			return c;
		case 11: return "J";
		case 12: return "Q";
		case 13: return "K";
		default: return "0";
 }
}

//checks if index hs already been used to get card from deck array
int checkIfUsed(const int index) {
	for(int i = 0; i < used_index; i++) {
		if(index == used[i]){
			return -1;
		}
	}
	return index;
}

//finds unused random index for deck array and returns it
int getCardIndex(void) {
	int index;
	//cehck a random number between 0 and 51. -1 means number has already been used
	while((index = checkIfUsed(rand() % 52)) == -1) {
		//loop through till find an unused index
	}
	used[used_index++] = index;
	return index;
}

//make draw pile from remaing cards
void initDrawPile(void) {
	deck_win = newwin(11,23,14,94);
   	draw_pile = deck[getCardIndex()];
	draw_pile->stack_index = -1;
	Card *temp = draw_pile;
	while(used_index < 52) {
	   temp->next = deck[getCardIndex()];
	   temp = temp->next;
	}
	current_card = draw_pile;
	printCard(current_card,0,0,deck_win);
	current_card->show = YES;
	printCard(current_card,0,11,deck_win);
	wrefresh(deck_win);	
}

//create columns of cards 
void initColumns(void) {
	srand(time(NULL)); //seed random number generator
	for(int i = 0; i < 7; i++) {
		columns[i] = malloc(SIZE_COLUMN);	
		columns[i]->column_win = newwin(50,11,0,(i)*11);
		columns[i]->bottom = i;
		for(int j = 0; j < i + 1; j++) {
			columns[i]->stack[j] = deck[getCardIndex()]; //get a random card from deck
			columns[i]->stack[j]->column_index = i;
			columns[i]->stack[j]->stack_index = j;
		}
		columns[i]->stack[i]->show = YES; //bottom card for that column should be showing face value. 
	}
}


//create all 52 cards
void initDeck(void) {
	enum Suites suite[] = {Hearts,Diamonds,Clubs,Spades}; //array holding suite values
	int suite_i = -1;  //index for array of suite values
	for(int i = 0; i < 52; i++) {
		deck[i] = malloc(SIZE_CARD);
		deck[i]->value = (i%13)+1;  //range of numbers from 1 to 13
		snprintf(deck[i]->face,3,"%s",getFace(deck[i]->value));
		if(deck[i]->value == 1) { //switch to next suite in array with each ace. 
			suite_i++; //increment array index
		}
		deck[i]->suite = suite[suite_i];
		deck[i]->show = NO;
	}
}

void initDiscardPiles(void) {
	for(int i = 0; i < 4; i++) {
		discard[i] = malloc(SIZE_COLUMN);	
		discard[i]->column_win = newwin(9,13,0,80+(i*14)); //make window for discard pile
    	wborder(discard[i]->column_win,'|','|','-', '-', '+', '+', '+', '+'); //create border for discard piles
		wrefresh(discard[i]->column_win); //display window
		discard[i]->bottom = 0; //set stack index for discard pile to 0
		discard[i]->stack[0] = malloc(SIZE_CARD); //init bottom of discard with dummy card
		discard[i]->stack[0]->value = 0; //initial card value is 0, for stacking ace as first real card
		discard[i]->stack[0]->suite = Dummy;  //suite is dummy so that any suite can stack ontop of it as first card.
	}

}

void initColors(void) {
	if(has_colors()) { //if terminal can display color
		start_color();
		init_pair(Hearts,COLOR_MAGENTA,COLOR_BLACK); //also color for Diamonds
		init_pair(Clubs,COLOR_CYAN,COLOR_BLACK); //aslo color for Spades
	}
}

void initScreen(void) {
	initscr();        //start ncurses
	noecho();	      //dont display key strokes
	cbreak();	     //disable line buffering
	curs_set(0);    //hide cursor
	keypad(stdscr,TRUE);  //enable keypad, needed for mouse.
	mousemask(ALL_MOUSE_EVENTS,NULL); //enable all mouse events
	refresh();
}

void initGame(void) {
	initDeck();
	initColumns();
	initDrawPile();
	initDiscardPiles();
	printAllColumns();
}

int main(void) {
	initScreen();
	initColors();
	initGame();
	gameLoop();
	endwin();
	return 0;
}
