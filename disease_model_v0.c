/* bTB simulation models
1. Each herd has 3 production types: calf, heifer, adult. This is set as an array.
2.  in each production type, struct of animals are connected.
3. These structs move between production types if necessary.
4. Struct also moves between farms.

=====================================================
== Data =============================================
There are two pointers of farm management unit.
1. Pointer to struct of animals.
2. Pointer to data storing infection status and other info of farms.


Farm data (dynamic memory)
Animal (dynamic memory)
Movement (dynamic memory)

*/

/*
--------------------------------------------------------------------
Production types in farms
*/

/* C LIBRARIES TO INCLUDE */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <malloc.h>

/* STRUCTURE DECLARATIONS----------------------------------------------------- */  
  struct animal_node {
      long long akey ; // animal id
      int age_day;   /* age in months or day*/
      int type;         // production type, 0 = calf, 1 = heifer, 2 = adult
      int sex;          /* sex, female = 1 male = 0*/
      int breed;
      int tb_status;    /* TB status. Sus = 0, Exposed = 1, Occult =2, Detectable =3. Occult and Detectable are infectious*/
      int pregnant_status; /* Pregnant status positive = 1*/
      int num_births; // record how many births it gave to
      struct animal_node *previous_node ;/*pointer to the previous node*/
      struct animal_node *next_node; /* pointer to next animal*/
   }; 
   
   struct event_node {
      int event_type;   
	  /*event type: 0 calf movement
	                1 heifer movement
					2 adult movement
					3 new birth
					4 cull/death
					5 test 
					6 exposed to occult
					7 occult to detectable*/
      long long akey ; //animal id
      int src_pro_id;  // production unit id where the event occurs, if it's movement then it's source farm
      int des_pro_id; 
	  int src_testarea;         
      struct event_node *next_node; /* pointer to next event*/
   }; 
   
   
/* FUNCTION DEFINITIONS */
  
void read_farm_data(); //Function for reading the data
void read_movement_data() ;
void read_animal_data();
void read_test_data();
void add_event_node();
void add_animal_node();
void visualize_list();
void visualize_animals() ;
void move_animal_unit();
void read_birth_data();
void test_farms();
 
/*   VARIABLES DECLARATION--------------------------------------------------------------------*/

/* Variables related to farm and FarmProductionStatus*/
int const num_production_type = 3;
//int num_farms = 16950; // number of farms present at 2000 july 1
int const num_total_farms = 45965; // this is the total number of herds that appeared in movement\longvity table
int const num_farm_production = 45965*3 ; // This is the ID for each farm production unit
int num_farm_var = 7 ; // 6 variables for each farm; farm_id, x, y, DCA, tb_status, testing schedule, tb_detected
char FarmDataFile[] = "/C_run/all_herds_details_export.csv";

int column_prostatus = 7 ; // number of variable in the table of infection status
// at this monemnt; N, sus, exposed, occult, detectable,if farm detected or not, infection pressure,

/* Variables related to individual animal*/
int num_animals = 3624420 ; // as of 2000 July 1st
int num_animals_var = 8; // akey, farm_id, age_day, age_type, sex, breed, pregnant_status, tb_status
char AnimalDataFile[] = "/C_run/tanimals_present_1july2000_toy.csv";

int num_total_animals = 16534951;//storage for animals

/* Variables related to events(movement, birth,test)*/
char MoveDataFile[] = "/C_run/tmovements_since2000july_export.csv";
int num_moves = 9681150;
int num_moves_vars = 6; // serial_akey, date, src_serial_hid, des_serial_hid, age_type, src_testarea

char BirthDataFile[] = "/C_run/birth_table_since2000july.csv";
int num_births = 9667100 ;
int num_births_vars = 5;// akey, bdate, src_farm, sex, breed

char TestDataFile[] = "/C_run/tb_test_schedule_export.csv";
int num_tests = 742 ;
int num_tests_vars = 2;

/* Variables related to simulations*/
int sim_years = 5;
int sim_days= 5*365; // 10 years
long long i, j;

/* Variables related to disease*/
int day_to_detect, day_to_occult;
int max_day_detect = 0.4*365; // max day to detect - 1 Conlan(2014) modified
int max_day_occult = 11.1*365; // max day to occult - 1  Brooks-Pollock's parameter
double Se_occult = 0.5;
double Se_detect = 0.8;

// int temp_id = 0 ; //temporary farm id
// int transition_id = 0; // this is the row number for table that stores disease trnsition events

// disease transition events include transition from exposed to Occut, and Occult to infectious
// this table will be later used to add transition events to day_event pointer


/* what's the problem about not tracing non-infected animals?
For instance, we can update the status of a farm when they receive bTB infected animals
and then adjust the age structure and herd size of farm.
By doing this we assume herd size stays the still and age structure is the same.
But can we really know if the herd size is going to be increased or not?
If not this assumption seems to be ok*/

/* MAIN PROGRAM */
int main(void){
	
       
    
/*=========================DATA PREPARATION=====================================================*/      
       
    /* 2. Read Data*/
	/* Following creates data or structure as follows.
	(1) FarmData (stores farm variable e.g. xy, bTB status, testing area, farmer type)
	(2) Farm_pointer (stores pointers to the first animal on a given farm with a given management type)
	(3) Read animal data
	(4) MoveData (Movement data)
	*/  
       
    	/*==========2.1  Read in Farm Data=============================================================== */
    	// This is the basic farm data such as number of cattle in each management status
   	 	/* First create array FarmData, which specifies the row memory size*/
   	  	 double **FarmData = (double**)malloc( sizeof(double *) * num_total_farms);
   	  	 for(i = 0; i < num_total_farms; i++)
      	  {
     	       /*Then, for each row, specifies the required memory size for column*/
     	     FarmData[i] = (double*)malloc( sizeof(double) * num_farm_var); 
			  /* what kind of farm variable do we need?*/ 
     	   }
      	
      	/*2.1.2 Read FarmData */
     	read_farm_data(FarmDataFile, FarmData, num_total_farms);
		 printf("farm data read"); 
     	/*==================================================================================================*/
    

        
		/*=============================================================================*/
		
		        
		/*=====2.3 Read in animal Data===========================================================*/
		/* How to set the initial animal that are present at a given farm?*/
		/* Do we really need to use the real data? Just can we model the age distribution?*/
		/* Needs to create an algorithm that can define which animals are on which farm on a given time in LIC data*/
       		double **AnimalData = (double**)malloc( sizeof(double *) * num_animals);
   	  		 for(i = 0; i < num_animals; i++)
      	 	 {
     	       /*Then, for each row, specifies the required memory size for column*/
     	     AnimalData[i] = (double*)malloc( sizeof(double) * num_animals_var); 
			  /* what kind of farm variable do we need?*/ 
			  //so far, age in months, type, sex, tb_status, pregnant_status
     	   		
			}
					
      	read_animal_data(AnimalDataFile,AnimalData,num_animals) ;
      	
      	printf("animal data read");
      /* Check if it reads AnimalData properly
	  	printf("Animal id is %lld",(long long) AnimalData[0][0]); // intersting it's now working, why do we need declaration?
        long long temp = AnimalData[0][0];
        int temp2 = AnimalData[0][1];
        printf("Animal id and farm is %lld, %d", temp, temp2);
      	system("pause") ;*/
      	
    /*2.4 Read movement data*/
   	  	 double **MoveData = (double**)malloc( sizeof(double *) * num_moves);
   	    	for(i = 0; i < num_moves; i++)
   	    	 {
   	    	 MoveData[i] = (double*)malloc( sizeof(double) * num_moves_vars);  
   		     	}
   	    	read_movement_data(MoveDataFile, MoveData, num_moves);
   	    	printf("move data read");
   	    	
   	/* 2.5 Read test data*/
   	double **TestData = (double**)malloc( sizeof(double *) * num_tests);
   	    	for(i = 0; i < num_tests; i++)
   	    	 {
   	    	 TestData[i] = (double*)malloc( sizeof(double) * num_tests_vars);  
   		     	}
   	    	read_test_data(TestDataFile, TestData, num_tests);
   	    	printf("test data read");
   	    	
   	    	
    /*Read birth data*/
    double **BirthData = (double**)malloc( sizeof(double *) * num_births);
   	    	for(i = 0; i < num_births; i++)
   	    	 {
   	    	 BirthData[i] = (double*)malloc( sizeof(double) * num_births_vars);  
   		     	}
   	         read_birth_data(BirthDataFile, BirthData, num_births) ;
   	         printf("birth data read");
      	/*============================================================================================*/
 
/* Set memory for linked lists*/      
struct animal_node** animal_node_pointer = (struct animal_node**)malloc( sizeof(struct animal_node*) * num_total_animals);	  	
double **FarmProductionStatus = (double**)malloc( sizeof(double *) * num_farm_production);
struct animal_node* FarmProductionList[num_farm_production]; // pointer to first animal at each farm-production
struct event_node *event_day[sim_days]; // a pointer to a struct

/*===========================================================================================
Itearation starts from here===========================================*/      	
      srand((unsigned)time(NULL));	
      	
      	
      	
      	// INITIALISE THE FARM TB DETECTED STATUS, SUM OF OCCULT, SUM OF DETECTABLE
      	for(i=0; i< num_total_farms; i++)
      	{
      		FarmData[i][6] == 0;
      		FarmData[i][7] == 0;
      		FarmData[i][8] == 0;
		  }
      	
      	/* Following codes to make linked lists need to be repeated in each iteration*/
      	/*=====Prepare Vector of day "event_day" that links to EventNode that stores information for any events (move/born/culling, infection status change)*/
      		  	/*=====2.2.1 Farm-production pointer 1: Pointer to table of infection status=======================*/
	  	// This is table storing number of infected, susceptible and so on for each management unit
	  	
	  		
   	  	 	for(i = 0; i < num_farm_production; i++)
      	  	{
     	       /*Then, for each row, specifies the required memory size for column*/
     	     FarmProductionStatus[i] = (double*)malloc( sizeof(double) * column_prostatus); 
     	     FarmProductionList[i] = NULL; // initialise the animal struct
			  /* what kind of farm variable do we need?*/ 
			  for (j=0; j < column_prostatus; j++)
			  {
			  	FarmProductionStatus[i][j] = 0 ; 
				  // initialise the status table, but keep herd id 
			  }
			  // then read the initial bTB status and total N
     	   	}
     	
     	/*====2.2.2 Farm-production pointer 2: Pointer to animal array===========================*/   	 		  
          for(i = 0; i < sim_days; i++)
                {
                event_day[i] = NULL;
                }
      	printf("event is fine");
      	/* Create a vector of size of number of animals that store a pointer to animal_node*/
      	
      	
		  for (i=0; i < num_total_animals; i++)
      	{
      		animal_node_pointer[i] = NULL;
		  }
        printf("animal_node_pointer is fine"); 
        /*===Populate animal arrays and fill the FarmProductionStatus as well=========================*/ 
          // Also here we can choose which animal is infected
          // Also we can count number of animas in each status if necessary to pass them to FarmProductionStatus
		  
		  struct animal_node *new_node;
		  struct event_node *new_event;
		   // each animal struct is also a pointer to a struct   
          for (i=0; i < num_animals; i++)
          { 
                
                new_node = (struct animal_node*)malloc(sizeof( struct animal_node )); 
                long long current_akey = (int)AnimalData[i][0];
                new_node -> akey = current_akey ; // extract from animal data
                new_node -> age_day = (int)AnimalData[i][2] ;
                
                
                int current_farm = (int)AnimalData[i][1] ; // farm id
                int current_type = (int)AnimalData[i][3] ; // which age group (production unit)
                int current_pro_id = current_farm*3 + current_type ;
                
                new_node -> sex = (int)AnimalData[i][4] ;
                new_node -> breed = (int)AnimalData[i][5];
            	new_node -> pregnant_status = 1;
            	new_node -> next_node = NULL;
            	
            /*If pregnant, determine when they give birth. This data can be stored at Event table rather than the linked animal list*/
           /* IGNORE PREGNANCY FOR NOW
		    if ((int)AnimalData[i][6] ==1) // if pregnant, then add an event list to the day
            {
            	int birth_date = 70;
            	
            	
            	new_event = (struct event_node*)malloc(sizeof( struct event_node ));
            	new_event -> akey = current_akey ;
            	
            	new_event -> production_id = current_pro_id ;
            	new_event -> event_type = 2; // 2 is giving birth
            	new_event -> next_node = NULL;
            	add_event_node(event_day,birth_date, new_event) ;
            	//visualize_list(event_day, 70);this works
            	//printf("This animal is %lld",current_akey);
          	
			}  
		
            */
          
            
            /*---------TB status BLOCK------------------------------------------------------------------------------*/
            // TB status - 2 ways, either extract from data or choose here
            /* Not relevant here but when updating infection pressure,
            it is easier to have an indicator if the number of occult/detectable
            are changed since previous update. Create a column called inf_chaned*/
            
                new_node -> tb_status = (int)AnimalData[i][7];
                // 0: susceptible 1: exposed 2: occult 3: infectious
            // Now if the cattle is exposed status, choose time to be occult
			if ((int)AnimalData[i][7]==1)
			{
				//randomly select from some distributions
				day_to_occult = rand()%max_day_occult +1  ; // randomly select a date to occult
				FarmProductionStatus[current_pro_id][2] = FarmProductionStatus[current_pro_id][2]+1; // increment by 1
				
				
				new_event = (struct event_node*)malloc(sizeof( struct event_node ));
				
				new_event -> akey = current_akey;
			//	printf("I %lld will become occult after %d days", current_akey, day_to_occult);
				new_event -> src_pro_id = current_pro_id ;
				new_event -> des_pro_id = -100 ;
				new_event -> event_type = 6; // 3 is exposed to occult
            	new_event -> next_node = NULL;
            	new_event -> src_testarea = -100 ;
            	add_event_node(event_day,day_to_occult, new_event) ;
            	/* Following was to check the bug in adding new event, which occurred because
            	I forgot to specify the size of new_event and then old memory was used
            	printf("My akey is %lld", current_akey) ;
            	system("pause") ;
            	printf("Day is %d", day_to_occult) ;
            	system("pause") ;
				visualize_list(event_day, day_to_occult);
				system("pause") ;*/
			}    
            
			
			// If animal is in occult then choose a date to be infectious
			if ((int)AnimalData[i][7]==2)
			{
				//randomly select from some distributions
				// for now draw a random value from uniform distribution
			
				day_to_detect = rand()%max_day_detect+1;
				FarmProductionStatus[current_pro_id][3]++; // increment by 1
				FarmData[current_farm][7] ++; //increase number of occult
				
				new_event = (struct event_node*)malloc(sizeof( struct event_node ));
				
				new_event -> akey = current_akey;
				new_event -> src_pro_id = current_pro_id ;
				new_event -> des_pro_id = -100 ;
				new_event -> event_type = 7; // 4 is occult to detectable
            	new_event -> next_node = NULL;
            	new_event -> src_testarea = -100 ;
            	add_event_node(event_day,day_to_detect, new_event) ;
			}
            
			// If animal is detectable
			if ((int)AnimalData[i][7]==3)
			{
				
				FarmProductionStatus[current_pro_id][4]++; // increment by 1
				FarmData[current_farm][8]++;
				} 
			// if animal is susceptible
			else if ((int)AnimalData[i][7]==0)
			{
				
				FarmProductionStatus[current_pro_id][1]++; // increment by 1
				} 
			// finally increment for the total number of animals in this type
			FarmProductionStatus[current_pro_id][0] = FarmProductionStatus[current_pro_id][0]+1;
			
			/*-------------TB STATUS BLOCK END--------------------------------------------------------------------*/
			
			// then initialise the next_node
                new_node -> next_node = NULL;   
                new_node -> previous_node = NULL ;
            /* ADD THE NEW NODE TO THE ARRAY */
                 add_animal_node(FarmProductionList, current_pro_id, new_node ) ;
                 
            /* ADD address of this animal to the pointer list*/
            animal_node_pointer[current_akey] = new_node ;
           } 
           printf("adding animal done");
           
           /* Check if animals were added properly
           for (i=0; i < 2; i++)
           {
           	visualize_animals(FarmProductionList,i) ;
		   }
		   system("pause");
		   */
           
        /*    for (i = 0 ; i < 40; i++)
      {
          visualize_list(event_day, i);
          }
          system("pause");*/
      
        /*=======================================================================================================*/      
      
     

   	    	// Movements are going to be probabilistic rather than explicitly 
   	    	// moving recorded animals?
   	    	// If so, we only need number of animals moved in each production type
   	    	
   	    //	printf("Movement data is like this %lld, %lld, %lld", (long long)MoveData[0][0], (long long)MoveData[1][0],(long long)MoveData[2][0]) ;
   	
    /*
    ------------------------------------------------------------------------------
    Above section should be one-off. Reading data is only once.
    ------------------------------------------------------------------------------

// here specifies iterations

/* 3 Updating status*/
	/* Procedure.
	1) Calculate the totatl RATE of Markov events (within-herd and wildlife-to-farm.
	2) Decide a date to next Markov event based on the RATE.
	3) If this date is earlier than next Non-Markov event, then do the Markov.
	4) Else, do Non-Markov.Update the event table.
	5) Update farm status, go back to 1). */
	
	
	/*So again, do we have to track all demographic and movements on uninfected farms?
	
	At least we need to know who is providing cattle to bTB infected farms.
	
	But how about false-positive? WHen it happens, their movements will stop for a while.
	Have to model test positive even bTB status is negative.
	Then when it happens stop moving and put culling.
	*/


/*===============================================================================================
==== CREATE AND UPDATE NON-MARKOV EVENTS
================================================================================================*/
int current_event_type ;
	/* Add movements to event array*/
	 // each farm struct is also a pointer to a struct   
          
		  for (i=0; i < num_moves; i++)
          { 
                
                new_event = (struct event_node*)malloc(sizeof( struct event_node )); 
                current_event_type =  (int)MoveData[i][4];
                new_event -> event_type = current_event_type ; // 0 is calf, 1 heifer 2 adult
                long long current_akey = (long long)MoveData[i][0] ;//akey
                int current_pro_id = (int)MoveData[i][2]*3 + current_event_type;
                
                new_event -> akey = current_akey;
                new_event -> src_pro_id = current_pro_id ;
				new_event -> des_pro_id =  (int)MoveData[i][3]*3+current_event_type; // destination farm
                new_event -> src_testarea = (int)MoveData[i][5] ;
				new_event -> next_node = NULL;   
         
                int current_day = (int)MoveData[i][1] ;
               
                /* ADD THE NEW NODE TO THE ARRAY */
                 add_event_node(event_day, current_day, new_event) ;
                 
           }
           
           
           
    /* ADD BIRTH EVENT AND ADD BIRTH ANIMALS To THE ANIMAL POINTER*/
    
          for (i=0; i < num_births; i++)
          { 
                long long current_akey = (long long)BirthData[i][0] ;//akey				
                int current_pro_id = (int)BirthData[i][2]*3 ;
                new_event = (struct event_node*)malloc(sizeof( struct event_node )); 
                new_event -> event_type = 3;          
                new_event -> akey = current_akey;
                new_event -> src_pro_id = -100 ;
				new_event -> des_pro_id = current_pro_id; // destination farm
                new_event -> src_testarea = -100 ;
				new_event -> next_node = NULL;   
         
                int current_day = (int)BirthData[i][1] ;
               
                /* ADD THE NEW NODE TO THE ARRAY */
                 add_event_node(event_day, current_day, new_event) ;
                 
            /* THEN CREATE ANIMAL NODE*/
                struct animal_node *new_animal;
                new_animal = (struct animal_node*)malloc(sizeof( struct animal_node ));
                new_animal->akey = current_akey;
                new_animal->age_day = 0;
                new_animal->type = 0;
                new_animal->num_births = 0;
                new_animal->breed = (int)BirthData[i][4];
                new_animal->sex = (int)BirthData[i][3];
                new_animal->pregnant_status = 0;
                new_animal->tb_status=0; //important assumption that vertical transmission does not occur
                new_animal->next_node = NULL;
                new_animal->previous_node = NULL;
                animal_node_pointer[current_akey] = new_animal; //now added memory location to the pointer list
                 
                
                 
           }
           
    /* ADD TESTING EVENTS*/
           for (i=0; i < num_tests; i++)
          { 
                
                new_event = (struct event_node*)malloc(sizeof( struct event_node )); 
                new_event -> event_type = -100;
                new_event -> src_pro_id = (int)TestData[i][1] ; //this is testing schedule
				new_event -> des_pro_id = -100; // destination farm
                new_event -> src_testarea = -100 ;
				new_event -> next_node = NULL;   
         
                int current_day = (int)TestData[i][0] ;
               
                /* ADD THE NEW NODE TO THE ARRAY */
                 add_event_node(event_day, current_day, new_event) ;
                 
           }
    
/*=========================DATA PREPARATION ENDS=============================================*/ 
//check if events were added properly
/* Following was used to check if events were properly added*/
	printf("adding events done");
	system("pause");	  
	  
 //for (i = 0 ; i < 12; i++)
   //   {
          visualize_list(event_day, 192);
     //     }
          printf("Before this you should have seen event lists!!");
system("pause")	;	 
//  */



/*=========================SIMULATION===================================================*/   
    

       
/* Below to check if non-Markov events in the event linked lists are working*/
/* TO DO*/
/*(1) NEEDS TO GIVE A DATE FOR NEXT NON-MARKOV EVENTS.
Checking each day if they have events and do these.
If reaches to the end of the day, then proceed to the next day when events occur.
Return the value for the date*/
int today_date = 0;
int next_nonMarkov_date,src_pro_id,des_pro_id,current_pro_id;
long long move_akey, animal_akey;

struct event_node *current_event ;
struct animal_node *current_animal ;
int temp_days = 31;
int detected_farm_id ;

printf("Before 2055") ;
visualize_animals(FarmProductionList,(2055*3+2)) ;
printf("These are animals in 2056 before") ;
visualize_animals(FarmProductionList,2056*3+2) ;
system("pause");


while(today_date<2) 
{// loop 1
	current_event = event_day[today_date] ; //this gets the first event on the day
if (current_event == NULL)
{//loop2
		today_date++; // go to the next day
		printf("go to next day") ;
}

else
{ //loop3
		next_nonMarkov_date = today_date; // this date will be compared to Markov event date
		//then here compare with Markov date and do Markov if Markov date < non-Markov
		//then do events
	if (current_event-> event_type <= 4 ) // if movement or new birth or cull death
	{
			/// here create move function
	//printf("Event is %d",current_event-> event_type);		
	move_animal_unit(FarmProductionList,FarmData,FarmProductionStatus,current_event,animal_node_pointer,Se_occult,Se_detect); // function to move animals
	//printf("movement done");		
	} // if movement done
	else if (current_event-> event_type ==5 )//if this is testing
	{
	printf("testing");		
	 // if movement done	
	test_farms(FarmData,current_event,Se_occult,Se_detect) ; // get the testing schedule id
		 // applies testing to all herds, for now only FarmProductionStatus[i[[5]>=1 farms
		 // calculate the p of missing all infected animals
		 // if false-positive, they test only reacter animals?
		 //then no-FP + no-detection, no-FP+detection, FP+no-detection, FP+detection
	printf("test done") ;	
	} 
		// if testing is happening, check all animals with test accuracy, then pick up test positive ones.
		//Record information of positive animal (age, region of herd etc) and cull it.
		//For positive herds, do follow up testing
	else if (current_event-> event_type == 6||current_event-> event_type == 7)
	{
	printf("updating TB status") ;
	animal_akey = current_event-> akey ;
	current_pro_id =  current_event-> src_pro_id;//// ;--needs age type
	current_animal = animal_node_pointer[animal_akey] ;
	int current_farm = (int)(floor(((double)current_pro_id)/3)) ;
		if (current_event-> event_type ==6 ) //exposed to occult
		{
		current_animal-> tb_status = 2 ;
		FarmProductionStatus[current_pro_id][2]--;
		FarmProductionStatus[current_pro_id][3]++;
		FarmData[current_farm][6]++;	
		}
		if (current_event-> event_type ==7) // occult to detectable
		{
		current_animal-> tb_status = 3 ;
		FarmProductionStatus[current_pro_id][3] = FarmProductionStatus[current_pro_id][3] - 1;
		FarmProductionStatus[current_pro_id][4]++;
		FarmData[current_farm][7]++ ;//increase detectable
		FarmData[current_farm][6]-- ;//decrease occult
		}
	printf("Updating TB status done") ;	
	}
		
	event_day[today_date] = current_event-> next_node ; //rewire to the next event
//	free(current_event) ; // free memory
			
                
			
		
}//loop3 ends
	
	
} // this is the end of loop 1 

//printf("These are animals in 2055") ;
//visualize_animals(FarmProductionList,2055*3+2) ;
//printf("These are animals in 2056") ;
//visualize_animals(FarmProductionList,2056*3+2) ;	   
	   
	   
	   
	       
} //END OF MAIN INT




/*==============================--SIMULATION DONE===========================*/

/*3.1 Markov events*/
/*========== WORKING



// Markov events include following events.
// (1) Within-herd transmission
int sus; // number of susceptible on a given farm
int infectious; // number of infected on a given farm
int exposed; // number of exposed on a given farm
int occult; // number of occult on a given farm
int N; // number of total animals in the management group
double beta_a; // within-herd transmission rate
double farm_inf_rate_total; // TOTAL Within-herd transmission rate
double wild_inf_rate_total; // TOTAL wildlife-transmission rate
double gamma; // transition rate from latent to occult 
double theta; // transition rate from occult to infectious

// (2) Change from latent to infectious 
// (3) Change from occult to detectable if needed
// (4) Transmission from wildlife

// Update TOTAL EVENT RATE

	for(i = 0; i < total_farm_production; i++)
	{
	// UPDATE EVENT RATE ON A GIVEN FARM-MANAGEMENT UNIT
	FarmManagement[i][5] = beta_a*FarmProductionStatus[1]*(FarmProductionStatus[3]+FarmProductionStatus[4])/FarmProductionStatus[0] ; // infection rate on a given farm
		
		// if wildlife infection is possible, activate the following
		// FarmManagement[i][6] = beta_b*sus ; // rate of wildlife transmission
		// UPDATE CUMULATIVE EVENT RATE ON THIS FARM
		FarmManagement[i][7] = current_total + FarmManagement[i][5]//+FarmManagement[i][8];
		// UPDATE CURRENT TOTAL EVENT RATE
		current_total = FarmManagement[i][7];
	}
	
	// NOW TOTAL EVENT RATE IS OBTAINED SO CALCULATE THE WAITING TIME
	// Generate a random value between 0 and 1
	double random_value = ((double)rand()/(double)RAND_MAX) ;
	delta_t = -log(random_value)/current_total ; // Waiting time
	
	// WAITING TIME < TIME TO NEXT NON-MARKOV EVENT
	if(delta_t < time_to_nonM) // if waiting time is shorter than time to the next non-Markov event
	{
		t = t + delta_t; // UPDATE TIME
		random_number = // SELECT RANDOM NUMBER
		markov_event_farm = // SELECT FARM IN WHICH EVENT OCCURS BASED ON CUMULATIVE VALUE
		markov_event = // SELECT EVENT TO OCCUR BASED ON EACH RATE ON THIS FARM
	}
	// WAITINg TIME > TIME TO NEXT NON-MARKOV EVENT
	else
	{
		t = t + time_to_nonM ; // UPDATE TIME TO NON-MARKOV EVENT DATE
		// THEN DO MARKOV EVENT
		nonmarkov_event_farm = event_table[][] // GET ALL btb INFECTED FARM THAT HAVE EVENTS
		nonmarkov_event =  // GET ALL EVENTS ON btb INFECTED FARM
		// UPDATE EACH FARM STATUS
	}
	=======================WORKING*/
	
	// NOW UPDATE THE EVENT TABLE SO THAT EVENTS THAT INVOLVE INFECTED FARMS ARE ONLY LISTED
	

	
	// then choose newly infected individuals randomly or following some rules
	// update their status - how to code choosing animals from arrayed structs?
	// then now have to trace the infection status of infected one to update their status 
	// when they become infectious from latent period.
	// Option1: define latent period first once they get infected and change status when it comes
	// Option2: Do Gllepise and set the date for the next event and when they occur choose event
	
	// Within-herd infection can be independently modelled unless movement occurs
	
	// Maybe the way is to look up the next global event which involves infected farm
	// then if the waiting time is after the global event, switch to the daily-basis event?
	// well I can't really think how to change the rate of events when global events occur.
	// So maybe better to update daily from beggining.
	// But because latent period is such long and transmission occurs so slow compared to
	// non-Markov events, maybe we just need to update at every non-Markov events.
	
	// Based on Andrew's note from workshop
	// Calculate waiting time and if this time is beyond the next non-Markov event
	// then do non-Markov and update time and get waiting time again.
	// else do Markov-event and update time and event.
	// Rate of event is the total rate of within-farm transmission and wildlife transmission
	// from all bTB infected and farms that can potentially infected from wildlife.
	// Just have to ask Andrew if using this approach is ok or not.
	
	
	
	// so do a bernouil trial using beta*S*I + beta_wildlife*S
	// Does each management group have different infection rate from wildlife?
	// Yes it should
	
	// repeat update everyday until some of the global events occur
	// Global events here include
	// on- and off-farm movement from bTB infected farm
	// culling and testing at infected farms
	// calving at the infected farms
	
/*3.2 Modelling management decision*/

	// decide when introduction and selling occur as a function of some farm characteristics
	// like high-turn over farm and trading farm
	// or auction date (which can be defined as priori because cattle move on the same date
	// so the precise date doesn't really matter
	// how about internet trading people? That would be much dependent on availability?
	// i.e. driven by the behaiour of source farm and traders
	
	// between management group occurs mandatory according to their age or months
	
	// need to know if move-in or move-out occurred on a given date.
	// update the herd size, number of infected and stuff
	// and go the next day
	
	// calving depends on calvng rate, which is a function of what?
	// does vertical transmission occurs if dam is btb infected and milk their calves?
	// culling depends on calving results
	


/*(2) DO THE JOB.
Until finding null, do the events.
What to do for each event.
Movement: Find the production unit and animal, detach from the linked list, add to the destination.
Status update: Find the animals and change the status. Record it. If exposed to occult/occult to infectious, create events.
Cull/Death: if TB related reason, keep the record. Maybe to farm linked list? Free memory of struct.
Birth: Increase the nth brith, then create new movement events to eter the production unit.*/

	
	
	
                
                
               
 
 
 
/* FUNCTIONS=============================================================================*/


/* -------------------------------------------------------------------------- */
/* read_farm_data: READING AND PARSING CSV FARM LIST */
/* -------------------------------------------------------------------------- */
void read_farm_data(char FarmDataFile[], double **FarmData,int num_farms)
{     
    /* OPEN INPUT FILE */
    FILE *Farms = fopen(FarmDataFile,"r"); 
       
    /* DECLARE STORAGE VARIABLES */
    int line_num, farm_id, testarea, tb_status,test_schedule_id;
    double x_coord, y_coord;
    
    /* READ LINES OF FARM FILE */
    for(line_num = 0; line_num < num_farms; line_num++)
      { 
         fscanf(Farms, "%d,%lf,%lf,%d,%d, %d", &farm_id, &x_coord, &y_coord, &testarea,&tb_status, &test_schedule_id);

         /* STORE VALUES IN FARM LIST */
             FarmData[line_num][0] = farm_id;
             FarmData[line_num][1] = x_coord;
             FarmData[line_num][2] = y_coord;
             FarmData[line_num][3] = testarea ;
             FarmData[line_num][4] = tb_status; 
             FarmData[line_num][5] = test_schedule_id;
             FarmData[line_num][6] = 0;//tb_detected or not
             FarmData[line_num][7] = 0;//sum of occult
             FarmData[line_num][8] = 0;//sum of detectable
             
      }            
   
   /* CLOSE INPUT FILE */
   fclose(Farms);
   
} 
/* -------------------------------------------------------------------------- */
/* READ INITIAL ANIMAL DATA*/
void read_animal_data(char AnimalDataFile[], double **AnimalData, int num_animals)
{
	
/* OPEN INPUT FILE */
    FILE *Animals = fopen(AnimalDataFile,"r"); 
       
    /* DECLARE STORAGE VARIABLES */
    int line_num, farm_id, age_day,Type, Sex, Breed, pregnant_status, tb_status;
    long long akey;
    
    /* READ LINES OF FARM FILE */
    for(line_num = 0; line_num < num_animals; line_num++)
      { 
         fscanf(Animals, "%lld,%d,%d, %d, %d,%d,%d,%d",&akey, &farm_id, &age_day, &Type, &Sex, &Breed,&pregnant_status,&tb_status );

         /* STORE VALUES IN AnimalData */
             AnimalData[line_num][0] = akey;
             AnimalData[line_num][1] = farm_id;
             AnimalData[line_num][2] = age_day;
             AnimalData[line_num][3] = Type ;
             AnimalData[line_num][4] = Sex; 
             AnimalData[line_num][5] = Breed;
             AnimalData[line_num][6] = pregnant_status;
             AnimalData[line_num][7] = tb_status; 
              
             
      }            
   
   /* CLOSE INPUT FILE */
   fclose(Animals);
   
} 

/*---------------------------------------------------------------------------*/
/* READ TESTING DATA*/
void read_test_data(char TestDataFile[], double **TestData, int num_tests)
{
	
/* OPEN INPUT FILE */
    FILE *Tests = fopen(TestDataFile,"r"); 
       
    /* DECLARE STORAGE VARIABLES */
    int line_num, day, test_schedule_id;
    
    /* READ LINES OF FARM FILE */
    for(line_num = 0; line_num < num_tests; line_num++)
      { 
         fscanf(Tests, "%d,%d",&day, &test_schedule_id);

         /* STORE VALUES IN AnimalData */
             TestData[line_num][0] = day;
             TestData[line_num][1] = test_schedule_id;
             
      }            
   
   /* CLOSE INPUT FILE */
   fclose(Tests);
   
}

 /* -------------------------------------------------------------------------- */
/* add_stub: ADD animals TO production_pointer */
/* -------------------------------------------------------------------------- */
void add_animal_node(struct animal_node *FarmProductionList[], int current_pro_id, struct animal_node *node_to_add )
{     

 struct animal_node *current_node1;
 current_node1 = FarmProductionList[current_pro_id];
 if(current_node1 == NULL)
    {
        FarmProductionList[current_pro_id] = node_to_add;
    }
 else
    {
       node_to_add -> next_node = current_node1;
       current_node1 -> previous_node = node_to_add ;
       FarmProductionList[current_pro_id] = node_to_add;
       
    }

}


 /* -------------------------------------------------------------------------- */
/* add_stub: ADD event TO event_day POINTER */
/* -------------------------------------------------------------------------- */
void add_event_node(struct event_node *event_day[], int day, struct event_node *node_to_add )
{     

 struct event_node *current_node1;
 // printf("event day is %p", event_day[day]);
 
 current_node1 = event_day[day];
 if(current_node1 == NULL)
    {
    	// printf("current node is null and then connect to %lld \n", node_to_add -> akey);
        event_day[day] = node_to_add;
        // printf("event day is now pointing to %p", event_day[day]);
    }
 else
    {
    	// printf("current node is %lld", current_node1 -> akey);
       node_to_add -> next_node = current_node1;
       //	printf("current node is now %lld", current_node1 -> akey);
       event_day[day] = node_to_add;
       // printf("current node is now %p",event_day[day]);
    }

}


/*------------------------------------------------------------------------------
READ MOVEMENT DATA
-------------------------------------------------------------------------- */
void read_movement_data(char MoveDataFile[], double **MoveData, long int num_moves)
{     
    /* OPEN INPUT FILE */
    FILE *Moves = fopen(MoveDataFile,"r"); 
       
    /* DECLARE STORAGE VARIABLES */
    long long line_num, akey;
    int src_farm, day, des_farm, age_type, src_testareanum;
    
    /* READ LINES OF FARM FILE */
    for(line_num = 0; line_num < num_moves; line_num++)
      { 
         fscanf(Moves, "%lld,%d,%d,%d, %d, %d",&akey, &day, &src_farm, &des_farm, &age_type,&src_testareanum);

         /* STORE VALUES IN FARM LIST */
             MoveData[line_num][0] = akey;
             MoveData[line_num][1] = day;
             MoveData[line_num][2] = src_farm;
             MoveData[line_num][3] = des_farm;
             MoveData[line_num][4] = age_type;
             MoveData[line_num][5] = src_testareanum;
      }
   /* CLOSE INPUT FILE */
   fclose(Moves);
   
} 
/*-----------------------------------------------------------------------------*/

/* READ BIRTH DATA AND MAKE EVENTS NODE*/
void read_birth_data(char BirthDataFile[], double **BirthData, long int num_births)
{     
    /* OPEN INPUT FILE */
    FILE *Births = fopen(BirthDataFile,"r"); 
       
    /* DECLARE STORAGE VARIABLES */
    long long line_num, akey;
    int bdate, farm, sex, breed;
    
    /* READ LINES OF FARM FILE */
    for(line_num = 0; line_num < num_births; line_num++)
      { 
         fscanf(Births, "%lld,%d,%d,%d,%d",&akey, &bdate, &farm, &sex, &breed);
         
             BirthData[line_num][0] = akey;
             BirthData[line_num][1] = bdate;
             BirthData[line_num][2] = farm;
             BirthData[line_num][3] = sex;
             BirthData[line_num][4] = breed;
      }
      fclose(Births);
}
/*--------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------
-- Random seed
----------------------------------------------------------------------------------*/
/*
int random_farm_seed(double P, int num_farms,double **FarmData, double **FarmProductionStatus)
{
	int num_p = roundf(P*num_farms) ;
	
	int i=0, j;
	int array[num_p];
	int current_farm;
	int current_herd_size;
	while(i<num_p)
	{
		array[i] = rand()%(num_farms-1);
		for(j=0;j<i;j++)
		if(array[j]==array[i])
		break;
		if(!(j<i))
		{
		current_farm = array[i];
		FarmData[current_farm][8] = 1;
		int rand_num = rand()%99;
		int indicate;
		if(rand_num<33)
		{
			indicate = 0;//infected is calf unit
		}
		else if (rand < 67)
		{
			indicate = 1;//infected is heifer unit
		}
		else 
		{
		indicate = 2;//infected id adult unit - they may not have the equal probability of infection
		}
		FarmProductionStatus[current_farm+indicate][1] = 1; //infect this management unit
		// so now allocated infection to FarmData and FarmProductionStatus
		// then we have to select infected animal on the infected premises
		// this will be perhaps easier to do on the linked list
		i++;
		}
		
	}
	
}
*/
/*=============================================================================
-- 
*/ 
/* -------------------------------------------------------------------------- */
/* VISUALIZE INFORMATION FROM LINKED LIST ----------------------------------- */
/* -------------------------------------------------------------------------- */
void visualize_list(struct event_node *day_list[], int day)  
{

 
 struct event_node *current_node1;
 current_node1 = day_list[day];
 if(current_node1 != NULL)
    {
       printf("Day %d: ", day );
       while(current_node1 != NULL)
          {
          	if (current_node1 -> event_type==5)
              {
			  printf("%d,%d ", current_node1 -> event_type, current_node1 -> src_pro_id);
		    }
              //system("pause") ;
              current_node1 = current_node1 -> next_node; 
              
          }  
       printf("\n");
   }
   
 

}

/* -------------------------------------------------------------------------- */
/* VISUALIZE LINKED ANIMALS ----------------------------------- */
/* -------------------------------------------------------------------------- */
void visualize_animals(struct animal_node *FarmProductionList[], int production_id)  
{

 
 struct animal_node *current_node1;
 current_node1 = FarmProductionList[production_id];
 if(current_node1 != NULL)
    {
       printf("Id is %d: ", production_id);
       while(current_node1 != NULL)
          {
              printf("%lld ", current_node1 -> akey);
              //system("pause") ;
              current_node1 = current_node1 -> next_node; 
              
          }  
       printf("\n");
   }
   
 

}

/* -------------------------------------------------------------------------- */
/* MOVE ANIMAL NODE FROM ONE TO OTHER FARM */
/* -------------------------------------------------------------------------- */
void move_animal_unit(struct animal_node *FarmProductionList[],double **FarmData,double **FarmProductionStatus, struct event_node *current_event, struct animal_node *animal_pointer_list[],double Se_occult, double Se_detect)
{
	int current_event_type = current_event->event_type;
	
	int src_pro_id = current_event->src_pro_id ;
	//printf("src_pro_id is %d", src_pro_id);
	long long current_akey = current_event->akey ;
	//printf("src_pro_id is %d", current_akey);
	int stop = 0;
	struct animal_node *moving_animal;
	moving_animal = (struct animal_node*)malloc(sizeof( struct animal_node ));
	struct animal_node *added_animal;
	added_animal = (struct animal_node*)malloc(sizeof( struct animal_node )); 
	
  if (current_event_type != 3) // if not 3 it means it is not new born
  {
    //printf("Not new calve");  
	int src_farm_id = (src_pro_id - current_event_type)/3 ;
	//printf("farm id is %d", src_farm_id);
/* Check if movement is allowed*/	
	if (current_event_type<=2 & FarmData[src_farm_id][6]==1) // if TB status is detected here, cancel
	{
		stop == 1;
	}
	else if ((current_event_type==1||current_event_type==2) && current_event->src_testarea == 0)
	{
		//pre-movement test
		
		int num_occult = FarmProductionStatus[src_pro_id][3] ;
		int num_detectable = FarmProductionStatus[src_pro_id][4] ;
		if (num_occult+num_detectable>0)
		{
			double P_miss = (pow(1-Se_occult,num_occult))*(pow(1-Se_detect,num_detectable))*100 ;
			if (rand()%100 > P_miss)
			/// needs to complete
			{
				FarmData[src_farm_id][6]=1 ; //farm becomes detected
				// now omit the function which animals to be detected
				//in future to add the detected animals to "detected list"
				stop == 1 ;
			}
			
		}
	}
	//printf("movement stop is %d",stop);
/*Check if movement is allowed ENDS*/
	//except above two conditions, allow movements

if (stop == 0) // if the movement is still allowed
{


/* Decide if the moving animal is new or not*/
    if (animal_pointer_list[current_akey]==NULL)
	{
	//	printf("this is new animal");
/*If this animal is new*/
		
		//printf("current akey is %lld",current_akey);
		
		added_animal->age_day = 0;
		added_animal->akey = current_akey;
		//printf("b");
		added_animal->type = current_event_type ;
		//printf("c");
		added_animal->tb_status = 0 ;
		//printf("d");
		added_animal->breed = 6 ;
		//printf("e"); 
		added_animal->num_births = 0;
		//printf("f");
		added_animal->sex = 1;
		//printf("g");
		added_animal->pregnant_status = 0;
		//printf("h");
		added_animal->next_node = NULL;
		//printf("i");
		added_animal->previous_node = NULL;
		//printf("new animal node is made");
		animal_pointer_list[current_akey] = added_animal;
		
		//printf("new animal is added");
	}
/*If this animal is new end*/
	else
	
/*If this animal is already existing*/
	{//LOOPB
	//printf("this is not new animal");
	if (FarmProductionList[src_pro_id] != NULL) // double check if there are animals
       {//LOOPA
  	
  	       
	       moving_animal = animal_pointer_list[current_akey] ;
	    //   printf("this animal is %lld", moving_animal->akey) ;
	       if (moving_animal -> previous_node != NULL)
	          { // if moving animal's previous node is conencted to other animal
	    //      printf("A starts");
			  struct animal_node *prev_animal;
              prev_animal = moving_animal -> previous_node ; // get the node which moving one is connected
	            if (moving_animal -> next_node != NULL) // if moving one's next node is conencted
	               {
	    //           	printf("B starts");
	  	           struct animal_node *next_animal;
                   next_animal = moving_animal -> next_node ; // get the next animal
                   prev_animal -> next_node = next_animal;    // reconenct previous_animal's next node
                   next_animal -> previous_node = prev_animal;//similarly reconnect next_animal's previous node
		//           printf("B ends");
				   }	
		        else // if next node is null
		           {
		   	       prev_animal -> next_node = NULL ;
		           }
		//       printf("A ends");    
	           }
	        else // if previous node is null 
	           {
	    //       printf("C starts");	
	           if (moving_animal -> next_node != NULL) //and if next node is not null
	              {  
	  	          struct animal_node *next_animal;
                  next_animal = moving_animal -> next_node ;
                  next_animal -> previous_node = NULL;
                  FarmProductionList[src_pro_id] = next_animal;
		          }
		       else 
		          {
		   	      FarmProductionList[src_pro_id] = NULL ; 
		          }
		//        printf("C ends");  
		        }	
       }//LOOPA
       
	if (FarmProductionStatus[src_pro_id][0]>0)
	   {
	   FarmProductionStatus[src_pro_id][0] = FarmProductionStatus[src_pro_id][0] - 1;
	   int current_status = moving_animal -> tb_status ;
	   FarmProductionStatus[src_pro_id][current_status+1] = FarmProductionStatus[src_pro_id][current_status+1] - 1;	
	 //and if there are no more TB animal change farm status to 0
	   
	   }
    }//LOOP B
    

	
//Part B: add moving animals to its new farm
// this applies to no matter what the animal is

// how should I consider about new farm?
int des_pro_id = current_event->des_pro_id;
    if (FarmProductionList[des_pro_id] == NULL)
    {
    	FarmProductionList[des_pro_id] = moving_animal;
    	moving_animal -> previous_node = NULL;
    	moving_animal -> next_node = NULL;
	}
	else
	{
	   struct animal_node *new_next_animal ;
	   
	   	new_next_animal = FarmProductionList[des_pro_id] ;
	   	new_next_animal -> previous_node = moving_animal;
	   	moving_animal -> next_node = new_next_animal;
	   	moving_animal -> previous_node = NULL ;
	   	FarmProductionList[des_pro_id] = moving_animal ;
	}
	FarmProductionStatus[des_pro_id][0]++;
	int current_status = moving_animal -> tb_status ;
	FarmProductionStatus[des_pro_id][current_status+1]++;
	
	
   
} // if stop==0 ENDS

} //if (current_event_type != 3) ENDS

else if (current_event_type == 3) // if this is new born just add
{
int des_pro_id = current_event->des_pro_id;
    if (FarmProductionList[des_pro_id] == NULL)
    {
    	FarmProductionList[des_pro_id] = moving_animal;
    	moving_animal -> previous_node = NULL;
    	moving_animal -> next_node = NULL;
	}
	else
	{
	   struct animal_node *new_next_animal ;
	   
	   	new_next_animal = FarmProductionList[des_pro_id] ;
	   	new_next_animal -> previous_node = moving_animal;
	   	moving_animal -> next_node = new_next_animal;
	   	moving_animal -> previous_node = NULL ;
	   	FarmProductionList[des_pro_id] = moving_animal ;
	}
	FarmProductionStatus[des_pro_id][0]++;
	int current_status = moving_animal -> tb_status ;
	FarmProductionStatus[des_pro_id][current_status+1]++;	
} // new born ENDS

}
    
    
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* TESTING HERDS */
/* -------------------------------------------------------------------------- */
void test_farms(double **FarmData,struct event_node *current_event,double Se_occult, double Se_detect)
{
	int testschedule_id = current_event->src_pro_id;//NOTE: THIS IS NOT SRC ID, BUT TEST ID
	for (i=0; i<num_total_farms; i++)
	{
		if(FarmData[i][5]==testschedule_id) // if the farm has the same testing id
		{
			int num_occult = FarmData[i][7];
			int num_detectable = FarmData[i][8] ;
		  
		  if((num_occult+num_detectable)>0) // if this farm has more than 0 occult+detectable
		  {
		     double P_miss = (pow(1-Se_occult,num_occult))*(pow(1-Se_detect,num_detectable))*100 ;
			 if (rand()%100 > P_miss)
			  {
			  FarmData[i][6]=1 ; //farm becomes detected
				// now omit the function which animals to be detected
				//in future to add the detected animals to "detected list"
				
			  }
		  }	
		}
	}
}
	
