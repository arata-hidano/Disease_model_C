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
      long long current_pro_id ;
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
					7 occult to detectable
					8 Unit change*/
      long long akey ; //animal id
      long long src_pro_id;  // production unit id where the event occurs, if it's movement then it's source farm
      long long des_pro_id; 
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
int update_markov_date();
void count_farms_infected_detected();
int write_OutPut();
void move_production_type();
 
/*   VARIABLES DECLARATION--------------------------------------------------------------------*/

/* Variables related to farm and FarmProductionStatus*/
int const num_production_type = 3;
//int num_farms = 16950; // number of farms present at 2000 july 1
int num_total_farms = 45965; // this is the total number of herds that appeared in movement\longvity table
long num_farm_production = 45965*3 ; // This is the ID for each farm production unit
int num_farm_var = 9 ; // 6 variables for each farm; farm_id, x, y, DCA, tb_status, testing schedule, tb_detected
char FarmDataFile[] = "/C_run/all_herds_details_export.csv";

int column_prostatus = 7 ; // number of variable in the table of infection status
// at this monemnt; N, sus, exposed, occult, detectable,infection pressure,if number of occult/detectable has changed

/* Variables related to individual animal*/
long num_animals = 3624420 ; // as of 2000 July 1st
int num_animals_var = 8; // akey, farm_id, age_day, age_type, sex, breed, pregnant_status, tb_status
char AnimalDataFile[] = "/C_run/tanimals_present_1july2000_toy.csv";

int num_total_animals = 16534951;//storage for animals

/* Variables related to events(movement, birth,test)*/
char MoveDataFile[] = "/C_run/tmovements_since2000july_export.csv";
long num_moves = 9681150;
//long num_moves = 888677;
int num_moves_vars = 6; // serial_akey, date, src_serial_hid, des_serial_hid, age_type, src_testarea

char BirthDataFile[] = "/C_run/birth_table_since2000july.csv";
int num_births = 9667100 ;
int num_births_vars = 5;// akey, bdate, src_farm, sex, breed

char TestDataFile[] = "/C_run/tb_test_schedule_export.csv";
int num_tests = 742 ;
int num_tests_vars = 2;

/* Variables related to simulations*/
int sim_years = 5;
int sim_days= 365*3; // 10 years
//341 farms detected TB in this 3 years
long long i, j;
int iteration;
int tot_iterations = 1;
int num_OutPut = 2; //infected and detected
char OutPutFile[] = "/C_run/OutPut.csv";
/* Variables related to disease*/
int day_to_detect, day_to_occult;
int max_day_detect = 0.4*365; // max day to detect - 1 Conlan(2014) modified
int max_day_occult = 5*365; // max day to occult - 1  Brooks-Pollock's parameter 11.1
double Se_occult = 0.2;
double Se_detect = 0.85;
double beta_a = 0.01;//within-herd transmission parameter
double beta_b = 0.0001;//wildlife transmission parameter
 //must be random draw from some distribution 0.003 - 0.036 Brooks-Pollock



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
   	  	 long long **FarmData = (long long**)malloc( sizeof(long long *) * num_total_farms);
   	  	 for(i = 0; i < num_total_farms; i++)
      	  {
     	       /*Then, for each row, specifies the required memory size for column*/
     	     FarmData[i] = (long long*)malloc( sizeof(long long) * num_farm_var); 
			  /* what kind of farm variable do we need?*/ 
     	   }
      	
      	/*2.1.2 Read FarmData */
     	read_farm_data(FarmDataFile, FarmData, num_total_farms);
	//	 printf("farm data read"); 
     	/*==================================================================================================*/
    

        
		/*=============================================================================*/
		
		        
		/*=====2.3 Read in animal Data===========================================================*/
		/* How to set the initial animal that are present at a given farm?*/
		/* Do we really need to use the real data? Just can we model the age distribution?*/
		/* Needs to create an algorithm that can define which animals are on which farm on a given time in LIC data*/
       		long long **AnimalData = (long long**)malloc( sizeof(long long *) * num_animals);
   	  		 for(i = 0; i < num_animals; i++)
      	 	 {
     	       /*Then, for each row, specifies the required memory size for column*/
     	     AnimalData[i] = (long long*)malloc( sizeof(long long) * num_animals_var); 
			  /* what kind of farm variable do we need?*/ 
			  //so far, age in months, type, sex, tb_status, pregnant_status
     	   		
			}
					
      	read_animal_data(AnimalDataFile,AnimalData,num_animals) ;
      	
      //	printf("animal data read");
      /* Check if it reads AnimalData properly
	  	printf("Animal id is %lld",(long long) AnimalData[0][0]); // intersting it's now working, why do we need declaration?
        long long temp = AnimalData[0][0];
        int temp2 = AnimalData[0][1];
        printf("Animal id and farm is %lld, %d", temp, temp2);
      	system("pause") ;*/
      	
    /*2.4 Read movement data*/
   	  	 long long **MoveData = (long long**)malloc( sizeof(long long *) * num_moves);
   	    	for(i = 0; i < num_moves; i++)
   	    	 {
   	    	 MoveData[i] = (long long*)malloc( sizeof(long long) * num_moves_vars);  
   		     	}
   	    	read_movement_data(MoveDataFile, MoveData, num_moves);
   	    //	printf("move data read");
   	    //	printf("MoveData akey is %lld, %lld, %lld, %lld", (long long)MoveData[0][0],(long long)MoveData[1][0],(long long)MoveData[2][0],(long long)MoveData[3][0]);
   	/* 2.5 Read test data*/
   	double **TestData = (double**)malloc( sizeof(double *) * num_tests);
   	    	for(i = 0; i < num_tests; i++)
   	    	 {
   	    	 TestData[i] = (double*)malloc( sizeof(double) * num_tests_vars);  
   		     	}
   	    	read_test_data(TestDataFile, TestData, num_tests);
   	    //	printf("test data read");
   	    	
   	    	
    /*Read birth data*/
    long long **BirthData = (long long**)malloc( sizeof(long long *) * num_births);
   	    	for(i = 0; i < num_births; i++)
   	    	 {
   	    	 BirthData[i] = (long long*)malloc( sizeof(long long) * num_births_vars);  
   		     	}
   	         read_birth_data(BirthDataFile, BirthData, num_births) ;
   	    //     printf("birth data read");
   	         
   	/*Create OutPut data*/
    double **OutPut = (double**)malloc( sizeof(double *) *tot_iterations);
   	    	for(i = 0; i < tot_iterations; i++)
   	    	 {
   	    	 OutPut[i] = (double*)malloc( sizeof(double) *num_OutPut); 
				   		     }
      	/*============================================================================================*/
 
/* Set memory for linked lists*/      
struct animal_node **animal_node_pointer = (struct animal_node**)malloc( sizeof(struct animal_node*) * num_total_animals);	  	
double **FarmProductionStatus = (double**)malloc( sizeof(double *) * num_farm_production);
struct animal_node* FarmProductionList[num_farm_production]; // pointer to first animal at each farm-production
struct event_node* event_day[sim_days]; // a pointer to a struct

/*===========================================================================================
Itearation starts from here===========================================*/  
for(iteration=0; iteration<tot_iterations; iteration++)
{//iteration starts
		    	
      
	  srand((unsigned)time(NULL));	
      	
      	
  for(i=0;i<tot_iterations;i++)
  {
  for (j=0; j < num_OutPut; j++)
	{
	OutPut[i][j] = 0 ; 
	}
  }
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
      //	printf("event is fine");
      	/* Create a vector of size of number of animals that store a pointer to animal_node*/
      	
      	
		  for (i=0; i < num_total_animals; i++)
      	{
      		animal_node_pointer[i] = NULL;
		  }
      //  printf("animal_node_pointer is fine"); 
        /*===Populate animal arrays and fill the FarmProductionStatus as well=========================*/ 
          // Also here we can choose which animal is infected
          // Also we can count number of animas in each status if necessary to pass them to FarmProductionStatus
		  
		  struct animal_node *new_node;
		  struct event_node *new_event;
		   // each animal struct is also a pointer to a struct   
          for (i=0; i < num_animals; i++)
          { 
                
                new_node = (struct animal_node*)malloc(sizeof( struct animal_node )); 
                long long current_akey = (long long)AnimalData[i][0];
                new_node -> akey = current_akey ; // extract from animal data
                new_node -> age_day = (int)AnimalData[i][2] ;
                if(current_akey==2454078)
                {
                	printf("hi i'm here at %lld",(long long)AnimalData[i][1]);
				}
                
                long long current_farm = (long long)AnimalData[i][1] ; // farm id
                int current_type = (int)AnimalData[i][3] ; // which age group (production unit)
                long long current_pro_id = current_farm*3 + current_type ;
                new_node-> current_pro_id = current_pro_id; //initial farm
                
                new_node -> sex = (int)AnimalData[i][4] ;
                new_node -> breed = (int)AnimalData[i][5];
            	new_node -> pregnant_status = 1;
            	
            	
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
				
				if(day_to_occult<sim_days)
				{
				
				new_event = (struct event_node*)malloc(sizeof( struct event_node ));
				
				new_event -> akey = current_akey;
			//	printf("I %lld will become occult after %d days", current_akey, day_to_occult);
				new_event -> src_pro_id = current_pro_id ;
				new_event -> des_pro_id = -100 ;
				new_event -> event_type = 6; // 3 is exposed to occult
            	new_event -> next_node = NULL;
            	new_event -> src_testarea = -100 ;
            	add_event_node(event_day,day_to_occult, new_event) ;
            //	printf("add occult");
                }
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
				
				if(day_to_detect<sim_days)
				{
				
				new_event = (struct event_node*)malloc(sizeof( struct event_node ));
				
				new_event -> akey = current_akey;
				new_event -> src_pro_id = current_pro_id ;
				new_event -> des_pro_id = -100 ;
				new_event -> event_type = 7; // 4 is occult to detectable
            	new_event -> next_node = NULL;
            	new_event -> src_testarea = -100 ;
            //	printf("add detect");
            	add_event_node(event_day,day_to_detect, new_event) ;
			    }
			}
            
			// If animal is detectable
			if ((int)AnimalData[i][7]==3)
			{
				
				FarmProductionStatus[current_pro_id][4]++; // increment by 1
				FarmData[current_farm][8]++;
				//printf("this farm has detectable");
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
          // printf("next is %lld previous is %lld",animal_node_pointer[2454078]->previous_node->akey,animal_node_pointer[2454078]->next_node->akey);
         // if(animal_node_pointer[2454078]->previous_node==NULL)
         // {
         // 	printf("YES NULL");
		 // }
		 // if(animal_node_pointer[0]->previous_node==NULL)
         // {
         // 	printf("YES NULL2");
		  //}
		  // printf("adding animal done");
           //system("pause");
           
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
          //printf("MoveData akey is %lld, %lld, %lld, %lld", (long long)MoveData[0][0],(long long)MoveData[1][0],(long long)MoveData[2][0],(long long)MoveData[3][0]);
		  //system("pause");
		  for (i=0; i < num_moves; i++)
          { 
                int current_day = (int)MoveData[i][1] ;
               // printf("current day is %d",current_day);
               // system("pause");
                if(current_day<sim_days)
                {
				
               // new_event = (struct event_node*)malloc(sizeof( struct event_node )); 
                current_event_type =  (int)MoveData[i][4];
                struct event_node *new_event;
                new_event = (struct event_node*)malloc(sizeof( struct event_node ));
                new_event -> event_type = current_event_type ; // 0 is calf, 1 heifer 2 adult
                long long current_akey;
				current_akey = (long long)MoveData[i][0] ;//akey
				if(current_akey==2454078)
				{printf("hi day %d",current_day);
				}
                //printf("%d current akey is %lld",i, (long long)MoveData[i][0]);
                //system("pause");
                //printf("%d current akey is %lld",i, current_akey);
                //system("pause");
                long long current_pro_id = (long long)MoveData[i][2]*3 + current_event_type;
                
                new_event -> akey = current_akey;
                // printf("current akey is %lld", new_event -> akey);
                 //system("pause");
                new_event -> src_pro_id = current_pro_id ;
				new_event -> des_pro_id =  (long long)MoveData[i][3]*3+current_event_type; // destination farm
                new_event -> src_testarea = (int)MoveData[i][5] ;
				new_event -> next_node = NULL;   
         
                
                /* ADD THE NEW NODE TO THE ARRAY */
                 add_event_node(event_day, current_day, new_event) ;
                 if (animal_node_pointer[current_akey]==NULL)//if this animal does not exist initially
                 {
                 struct animal_node *new_animal;
                 new_animal = (struct animal_node*)malloc(sizeof( struct animal_node ));
                 new_animal->akey=current_akey;
                 new_animal -> current_pro_id = current_pro_id ; //only first movement
                 new_animal->breed=6;//now just assume it's unknown
                 new_animal->sex=1; //now assume all female but if needs to be precise, have to get the data from tlogevity and add to movement data
				 new_animal->tb_status=0; //unknown
				 new_animal->num_births=0; //unknown
				 new_animal->type=current_event_type;
				 new_animal->pregnant_status=0;
				 new_animal->next_node=NULL;
				 new_animal->previous_node=NULL;
				 animal_node_pointer[current_akey] = new_animal;
				// if(new_animal->akey ==2454078)
               // {
				
               // printf("previous is %lld next is %lld", new_animal->previous_node, new_animal->next_node);
          // }
				 }
				 
               }
           }
           //printf("adding movement done");
           //system("pause");
           
           
    /* ADD BIRTH EVENT AND ADD BIRTH ANIMALS To THE ANIMAL POINTER*/
    
          for (i=0; i < num_births; i++)
          { 
          int current_day = (int)BirthData[i][1] ;
          if(current_day<sim_days)
          {
          	long long current_akey = (long long)BirthData[i][0] ;//akey
		  if (animal_node_pointer[current_akey]==NULL)//if this animal does not exist initially
                 {
                				
                long long current_pro_id = (long long)BirthData[i][2]*3 ;
                struct event_node *new_event;
                new_event = (struct event_node*)malloc(sizeof( struct event_node )); 
                
                new_event -> event_type = 3;          
                new_event -> akey = current_akey;
                new_event -> src_pro_id = -100 ;
				new_event -> des_pro_id = current_pro_id; // destination farm
                new_event -> src_testarea = -100 ;
				new_event -> next_node = NULL;   
         
                
               
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
                new_animal->current_pro_id = current_pro_id;
				new_animal->next_node = NULL;
                new_animal->previous_node = NULL;
                animal_node_pointer[current_akey] = new_animal; //now added memory location to the pointer list
            }
                
           }
           }
           
    /* ADD TESTING EVENTS*/
           for (i=0; i < num_tests; i++)
          { 
                int current_day = (int)TestData[i][0] ;
                if(current_day<sim_days)
                {
				struct event_node *new_event;
                new_event = (struct event_node*)malloc(sizeof( struct event_node )); 
                
                new_event -> event_type = 5;
                new_event -> src_pro_id = (long long)TestData[i][1] ; //this is testing schedule
				new_event -> des_pro_id = -100; // destination farm
                new_event -> src_testarea = -100 ;
				new_event -> next_node = NULL;   
         
                
               
                /* ADD THE NEW NODE TO THE ARRAY */
                 add_event_node(event_day, current_day, new_event) ;
                }
           }
           int YEARS = 3;
           int current_day;
    /* ADD UNIT CHNAGE EVENTS*/
        for(i=0;i<YEARS;i++)
           {
           	current_day = 365*i + 10;
           	printf("current day is %d",current_day);
           	struct event_node *new_event;
                new_event = (struct event_node*)malloc(sizeof( struct event_node )); 
                
                new_event -> event_type = 8;
                new_event -> src_pro_id = -100 ; //this is testing schedule
				new_event -> des_pro_id = -100; // destination farm
                new_event -> src_testarea = -100 ;
				new_event -> next_node = NULL;   
         
                
               
                /* ADD THE NEW NODE TO THE ARRAY */
                 add_event_node(event_day, current_day, new_event) ;
		   }
    
/*=========================DATA PREPARATION ENDS=============================================*/ 
//check if events were added properly
/* Following was used to check if events were properly added*/
	//printf("adding events done");
	//system("pause");	  
	  
 //for (i = 0 ; i < 12; i++)
   //   {
      //    visualize_list(event_day, 0);
     //     }
        //  printf("Before this you should have seen event lists!!");
//system("pause")	;	 
//  */


//visualize_animals(FarmProductionList,7186*3+1);
//system("pause");
/*=========================SIMULATION===================================================*/   
    

       
/* Below to check if non-Markov events in the event linked lists are working*/
/* TO DO*/
/*(1) NEEDS TO GIVE A DATE FOR NEXT NON-MARKOV EVENTS.
Checking each day if they have events and do these.
If reaches to the end of the day, then proceed to the next day when events occur.
Return the value for the date*/
int today_date = 1;
int next_non_markov_date,day_to_markov,updated_date;
long long move_akey, animal_akey;
double sum_inf_pressure;
struct event_node *current_event ;
//current_event = new_event = (struct event_node*)malloc(sizeof( struct event_node ));
struct event_node *adding_new_event ;

struct animal_node *current_animal ;
int temp_days = 31;
int current_pro_id ;
int j = 0;
int YEAR = 0;
//printf("Before 2055") ;
//visualize_animals(FarmProductionList,(2055*3+2)) ;
//printf("These are animals in 2056 before") ;
//visualize_animals(FarmProductionList,2056*3+2) ;
//system("pause");

//visualize_animals(FarmProductionList,1225*3+1);
//		system("pause");

//visualize_list(event_day,728);
//system("pause");
//	if(today_date==728)
//			{
//			for(i=1220;i<1226;i++)
//			{
  //          printf("id is %d", i);
	//		visualize_animals(FarmProductionList,i*3+1);
	//		system("pause");
	//	    }
	//	}

while(today_date<sim_days)
{
//	if(today_date==728||today_date==729)//727 ok 729 not ok, which means something happened on 728
//	{
//		for(i=1220;i<1226;i++)
//		{
//			printf("this is %d at day %d",i, today_date);
//		visualize_animals(FarmProductionList,i*3+1);
//		system("pause");	
//		}
//	}
	//{
	 //   printf("today is %d",today_date);
//		visualize_animals(FarmProductionList,1225*3+1);
//		system("pause");
//	}
	
	//printf("today is %d",today_date);
//	if(today_date==739)// 10, 375,.... between unit movement occurs
	//{printf("YEAR is now %d",YEAR);
	//	if(YEAR==2)
//		{
//	printf("yes");
//	visualize_animals(FarmProductionList,1225*3+1);
//	system("pause");
//		}
	//  move_production_type(FarmProductionStatus,FarmProductionList,num_total_farms);
	//  YEAR++;	
	//  printf("year is %d",YEAR);
	//}
  next_non_markov_date = today_date;
 // if(today_date>=720)
 // {
 // 	printf("that animal is in "animal_node_pointer[2454078]->current_pro_id
 // }
  while (event_day[next_non_markov_date] == NULL)
            {//loop2
		    next_non_markov_date++; // go to the next day
	//	    printf("go to next day") ;
            } // now get next non-markov date
  if(next_non_markov_date>=sim_days)
  {
  	break;
  }
  updated_date=update_markov_date(today_date,FarmData,FarmProductionStatus,FarmProductionList,num_farm_production,beta_a,beta_b,next_non_markov_date, event_day);
  //printf("day is updated %d",updated_date) ;
 // system("pause");
  if (updated_date==next_non_markov_date) // this means markov event did not happen
     {
	// printf("yes updated date is next non markov date");
     //system("pause");
	 // loop 1
	  //this gets the first event on the day
     current_event = event_day[next_non_markov_date];
    // printf("current_event is %d",current_event->event_type);
     //printf("current event is %d",current_event->event_type);
     //system("pause");
     while(current_event!=NULL)
     {
     	if(current_event->akey==1182045)
     	{
     		printf("this animal is from %lld to %lld with type %d",current_event->src_pro_id, current_event->des_pro_id,current_event->event_type);
		    printf("and current_pro_id is %lld and day %d",animal_node_pointer[1182045]->current_pro_id, today_date);
		    printf("previous animal is %lld and next animal is %lld",animal_node_pointer[1182045]->previous_node->akey,animal_node_pointer[1182045]->next_node->akey);
		 }
	 
	   if (current_event-> event_type <= 4 ) // if movement or new birth or cull death
	      {
			move_animal_unit(FarmProductionList,FarmData,FarmProductionStatus,current_event,animal_node_pointer,Se_occult,Se_detect); // function to move animals
	    //    printf("movement done akey is %lld",current_event->akey);
		if(current_event->akey==1182045)
     	{
     		printf("this animal is from %lld to %lld with type %d",current_event->src_pro_id, current_event->des_pro_id,current_event->event_type);
		    printf("and current_pro_id is %lld and day %d",animal_node_pointer[1182045]->current_pro_id, today_date);
		    printf("and next animal is %lld",animal_node_pointer[1182045]->next_node->akey);
		 }
		  
		
	      } // if movement done
	   else if (current_event-> event_type ==5 )//if this is testing
	      {
	   //  printf("testing");		
	 	   test_farms(FarmData,current_event,Se_occult,Se_detect) ; // get the testing schedule id
		 	 // if false-positive, they test only reacter animals?
		    //then no-FP + no-detection, no-FP+detection, FP+no-detection, FP+detection
	        //printf("test done") ;	
	        
	      } 
		    // if testing is happening, check all animals with test accuracy, then pick up test positive ones.
		    //Record information of positive animal (age, region of herd etc) and cull it.
		    //For positive herds, do follow up testing
	   else if (current_event-> event_type == 6||current_event-> event_type == 7)
	      {
	      printf("updating TB status") ;
	      animal_akey = current_event-> akey ;
	        current_animal = animal_node_pointer[animal_akey] ;
	        current_pro_id = current_animal->current_pro_id;
	        int current_farm = (int)(floor(((double)current_pro_id)/3)) ;
		    if (current_event-> event_type ==6 ) //exposed to occult
		      {
		      day_to_detect = rand()%max_day_detect+1;
		      int day_to_add = day_to_detect + next_non_markov_date;
			  
				  	
		      current_animal-> tb_status = 2 ;
		      FarmProductionStatus[current_pro_id][2]--;
		      FarmProductionStatus[current_pro_id][3]++;
		      FarmData[current_farm][7]++;
		      if(day_to_add<sim_days)
			  {
		      adding_new_event = (struct event_node*)malloc(sizeof( struct event_node ));
		      adding_new_event->akey=animal_akey;
		      adding_new_event->src_pro_id = -100; 
		      adding_new_event->des_pro_id = -100;
		      adding_new_event->src_testarea=-100;
			  adding_new_event->event_type=7;//occult to detectable happens
			  
			  add_event_node(event_day, day_to_add, adding_new_event) ;
		      }
			  
		      }
		    if (current_event-> event_type ==7) // occult to detectable
		      {
		      current_animal-> tb_status = 3 ;
		      FarmProductionStatus[current_pro_id][3]--;
		      FarmProductionStatus[current_pro_id][4]++;
		      FarmData[current_farm][8]++ ;//increase detectable
		      printf("detectable increased");
		     // system("pause");
		      FarmData[current_farm][7]-- ;//decrease occult
		      }
	        printf("Updating TB status done") ;	
	      }//exposed->occult ot occult->detectable DONE
	      else if (current_event->event_type==8)
	      {
	      	printf("Changing unit!");
	      	move_production_type(FarmProductionStatus,FarmProductionList,num_total_farms);
	      	printf("Changing unit done!");
	      	system("pause");
		  }
	      
	      
	    if(today_date==728)
     	{
     	//	printf("this animal is from %lld to %lld with type %d",current_event->src_pro_id, current_event->des_pro_id,current_event->event_type);
		   // printf("and current_pro_id is %lld",animal_node_pointer[2454078]->current_pro_id);
		   if(animal_node_pointer[1182045]->previous_node!=NULL&&animal_node_pointer[1182045]->previous_node->akey!=3546673)
		   {
		   	if(animal_node_pointer[1182045]->previous_node!=NULL)
		   	{
		   	printf("previous animal is %lld",animal_node_pointer[1182045]->previous_node->akey);
		   	
		   }
		    if(FarmProductionList[1220*3+2]!=NULL)
		   {
		   	printf("1220 is pointing %lld",FarmProductionList[1220*3+2]->akey);
		   }
		   else
		   {
		   	printf("1220 is pointing null");
		   }
		   	printf("current akey is %lld", current_event->akey);
		   	printf("And 1225 is pointing %lld",FarmProductionList[1225*3+1]->akey);
		   	//system("pause");
		   }
		   else if (animal_node_pointer[1182045]->previous_node==NULL)
		   {
		   	printf("1182045 is now first");
		   	if(FarmProductionList[1220*3+2]!=NULL)
		   {
		   	printf("and 1220 is pointing %lld",FarmProductionList[1220*3+2]->akey);
		   }
		   }
		   
		 }  
	      
		struct event_node *previous_event;
		//previous_event =  (struct event_node*)malloc(sizeof( struct event_node ));
	   previous_event = current_event ; //rewire to the next event
	   current_event = current_event->next_node;
	   if (current_event!=NULL)
	   {
	   free(previous_event);//this is not dynamic memory, but is it ok to free?
	   //printf("next event is %d, %lld", current_event->event_type, current_event->akey);
       }
	   event_day[next_non_markov_date] = current_event;
	   //printf("event day is pointing %lld",event_day[next_non_markov_date]->akey);
	   j++;
	   //printf("this is %d th event",j);
	  // system("pause");
     }//while loop for going to next events ends
	
	
     } // this is the end of loop 1    
     today_date = updated_date;
  }	
	
	
	



 


//printf("These are animals in 2055") ;
//visualize_animals(FarmProductionList,2055*3+2) ;
//printf("These are animals in 2056") ;
//visualize_animals(FarmProductionList,2056*3+2) ;	 
//for (i=0;i<num_total_farms;i++)
//{
//	if(FarmData[i][8]>0)
//	printf("this has detectable");
//  }  
	   
	   
count_farms_infected_detected(FarmData,OutPut,num_total_farms,iteration);
}//END OF EACH ITERATION
write_OutPut(OutPutFile,OutPut,tot_iterations,num_OutPut) ;    
} //END OF MAIN INT




	
	
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
	


	
                
                
               
 
 
 
/* FUNCTIONS=============================================================================*/


/* -------------------------------------------------------------------------- */
/* read_farm_data: READING AND PARSING CSV FARM LIST */
/* -------------------------------------------------------------------------- */
void read_farm_data(char FarmDataFile[], long long **FarmData,int num_farms)
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
void read_animal_data(char AnimalDataFile[], long long **AnimalData, int num_animals)
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
    //    printf("this was the first event of the day %d",day);
        // printf("event day is now pointing to %p", event_day[day]);
    }
 else
    {
    //	printf("current node is %lld", current_node1 -> akey);
       node_to_add -> next_node = current_node1;
    //   	printf("next node is now %lld", node_to_add -> akey);
       event_day[day] = node_to_add;
    //    printf("current node is now %lld",event_day[day]->akey);
    //    system("pause");
    }

}


/*------------------------------------------------------------------------------
READ MOVEMENT DATA
-------------------------------------------------------------------------- */
void read_movement_data(char MoveDataFile[], long long **MoveData, int num_moves)
{     
    /* OPEN INPUT FILE */
    FILE *Moves = fopen(MoveDataFile,"r"); 
       
    /* DECLARE STORAGE VARIABLES */
    long long line_num, akey,src_farm,des_farm;
    int  day, age_type, src_testareanum;
    
    /* READ LINES OF FARM FILE */
    for(line_num = 0; line_num < num_moves; line_num++)
      { 
         fscanf(Moves, "%lld,%d,%lld,%lld, %d, %d",&akey, &day, &src_farm, &des_farm, &age_type,&src_testareanum);

         /* STORE VALUES IN FARM LIST */
             MoveData[line_num][0] = akey;
             //printf("akey is %lld",(long long)MoveData[line_num][0]);
             //system("pause");
             //system("pause");
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
void read_birth_data(char BirthDataFile[], long long **BirthData, int num_births)
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
void visualize_list(struct event_node *event_day[], int day)  
{

 
 struct event_node *current_node1;
 current_node1 = event_day[day];
 //printf("current node is %lld",current_node1->akey);
 if(current_node1 != NULL)
    {
    //   printf("Day %d: ", day );
       while(current_node1 != NULL)
          {
          	if (current_node1->des_pro_id==(1225*3+2)||current_node1->src_pro_id==(1225*3+2))
              {
			  printf("%d,%lld ", current_node1 -> event_type, current_node1 -> akey);
			 // system("pause") ;
		    }
              
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

 int nth = 0;
 struct animal_node *current_node1;
 current_node1 = FarmProductionList[production_id];
 if(current_node1 != NULL)
    {
     //  printf("Id is %d: ", production_id);
       while(current_node1 != NULL)
          {
          //   if(current_node1->akey==2454078)
           //  {
			 
			 printf("nth %d current animal is %lld \n",nth, current_node1 -> akey);
             
             // system("pause") ;
         // }
              current_node1 = current_node1 -> next_node; 
              nth ++ ;
              
              
          }  
       printf("\n");
   }
   
 

}

/* -------------------------------------------------------------------------- */
/* MOVE ANIMAL NODE FROM ONE TO OTHER FARM */
/* -------------------------------------------------------------------------- */
void move_animal_unit(struct animal_node *FarmProductionList[],long long **FarmData,double **FarmProductionStatus, struct event_node *current_event, struct animal_node *animal_pointer_list[],double Se_occult, double Se_detect)
{
	int current_event_type = current_event->event_type;
	
	int src_pro_id = current_event->src_pro_id ;
	//printf("src_pro_id is %d", src_pro_id);
	long long current_akey = current_event->akey ;
	//printf("src_pro_id is %d", current_akey);
	int stop = 0;
	struct animal_node *moving_animal;
	//moving_animal = (struct animal_node*)malloc(sizeof( struct animal_node ));
	moving_animal = animal_pointer_list[current_akey] ;
	//struct animal_node *added_animal;
	//added_animal = (struct animal_node*)malloc(sizeof( struct animal_node )); 
//	if(current_akey==2454078)
//	{
//		printf("previous node is %lld and next node is %lld",moving_animal->previous_node, moving_animal->next_node);
//	}
	
  if (current_event_type != 3) // if not 3 it means it is not new born
  {
    //printf("Not new calve");  
	int src_farm_id = floor(src_pro_id/3) ;
	//printf("farm id is %d", src_farm_id);
/* Check if movement is allowed*/	
	if (current_event_type<=2 & FarmData[src_farm_id][6]==1) // if TB status is detected here, cancel
	{
		stop == 1;
	//	printf("stop farm is detected");
	//	system("pause");
	}
	else if ((current_event_type==1||current_event_type==2) && current_event->src_testarea == 0)
	{
		//pre-movement test
		
		int num_occult = FarmProductionStatus[src_pro_id][3] ;
		int num_detectable = FarmProductionStatus[src_pro_id][4] ;
		if (num_occult+num_detectable>0)
		{
			double P_miss = (pow(1-Se_occult,num_occult))*(pow(1-Se_detect,num_detectable)) ;
			if ( ((double)rand()/(double)RAND_MAX) > P_miss)
			/// needs to complete
			{
				FarmData[src_farm_id][6]=1 ; //farm becomes detected
				// now omit the function which animals to be detected
				//in future to add the detected animals to "detected list"
				stop == 1 ;
			//	printf("stop! pre movement detected");
			//	system("pause");
			}
			
		}
	}
	//printf("movement stop is %d",stop);
/*Check if movement is allowed ENDS*/
	//except above two conditions, allow movements

if (stop == 0) // if the movement is still allowed
{


/* Decide if the moving animal is new or not*/
    //if (animal_pointer_list[current_akey]==NULL)
//	{
	//	printf("this is new animal");
/*If this animal is new*/
		
		//printf("current akey is %lld",current_akey);
		
		//added_animal->age_day = 0;
		//added_animal->akey = current_akey;
		//printf("b");
		//added_animal->type = current_event_type ;
		//printf("c");
		//added_animal->tb_status = 0 ;
		//printf("d");
		//added_animal->breed = 6 ;
		//printf("e"); 
		//added_animal->num_births = 0;
		//printf("f");
		//added_animal->sex = 1;
		//printf("g");
		//added_animal->pregnant_status = 0;
		//added_animal->current_pro_id = current_event->des_pro_id;
		//printf("h");
		//added_animal->next_node = NULL;
		//printf("i");
		//added_animal->previous_node = NULL;
		//printf("new animal node is made");
		//animal_pointer_list[current_akey] = added_animal;
		
		//printf("new animal is added");
	//}
/*If this animal is new end*/
	//else
	
/*If this animal is already existing*/
	//{//LOOPB
	//printf("this is not new animal");
	//if (FarmProductionList[src_pro_id] != NULL) // double check if there are animals
      // {//LOOPA
  	
  	       
	       
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
       //}//LOOPA
       
	if (FarmProductionStatus[src_pro_id][0]>0)
	   {
	   FarmProductionStatus[src_pro_id][0] = FarmProductionStatus[src_pro_id][0] - 1;
	   int current_status = moving_animal -> tb_status ;
	   FarmProductionStatus[src_pro_id][current_status+1] = FarmProductionStatus[src_pro_id][current_status+1] - 1;	
	 //and if there are no more TB animal change farm status to 0
	   
	   }
    
    

	
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
	moving_animal->current_pro_id=des_pro_id;
	
   
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
	moving_animal->current_pro_id=des_pro_id;
} // new born ENDS

}
    
    
/* -------------------------------------------------------------------------- */
/* TESTING HERDS */
/* -------------------------------------------------------------------------- */
void test_farms(long long **FarmData,struct event_node *current_event,double Se_occult, double Se_detect)
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
		     double P_miss = (pow(1-Se_occult,num_occult))*(pow(1-Se_detect,num_detectable)) ;
			 if ( ((double)rand()/(double)RAND_MAX) > P_miss)
			  {
			  FarmData[i][6]=1 ; //farm becomes detected
				// now omit the function which animals to be detected
				//in future to add the detected animals to "detected list"
				
			  }
		  }	
		}
	}
}
/*----------------------------------------------------------------------------------*/
/* MOVING ANIMALS BETEWEEN PRODUCTION TYPE*/
/*----------------------------------------------------------------------------------*/
void move_production_type(double **FarmProductionStatus,struct animal_node *FarmProductionList[],int num_total_farms)
{
struct animal_node *current_animal;
struct animal_node *next_animal;
struct animal_node *target_animal;


int current_status;

 for (i=0; i<num_total_farms;i++)
     {
    	//printf("%d",i);
     	while(FarmProductionList[i*3+1]!=NULL)//if there are heifer
     	{
     	if(i>=1210 && i <= 1230&& FarmProductionList[i*3+1]->next_node!= NULL)
		    {
		    //	printf("doing %d", i);
		    //	printf("this animal is at %lld",animal)
		   // current_animal= FarmProductionList[i*3+1];
		   // if(current_animal->akey==2454078)
		    // {
		    // 	printf("Yes I am at %d",i) ;
			 //}
		    }
		//printf("not null %lld I is %d",FarmProductionList[i*3+1]->akey,i);
		//printf("next node is %lld",FarmProductionList[i*3+1]->next_node->akey);
		// system("pause");
	     //}
     //	current_animal = (struct animal_node*)malloc(sizeof( struct animal_node )); 
     //	next_animal = (struct animal_node*)malloc(sizeof( struct animal_node ));
     //	target_animal = (struct animal_node*)malloc(sizeof( struct animal_node ));
     	
		 current_animal = FarmProductionList[i*3+1]; //get the first animal
		 //if checking age, do here in future
		if(current_animal->akey==1182045)
		{
			printf("unit is %lld i is %d", current_animal->current_pro_id, i);
			//printf("this animal is now farm %lld and previous animal is %lld next is %lld",current_animal->current_pro_id,current_animal->previous_node->akey,current_animal->next_node->akey);
			if(current_animal->next_node!=NULL)
			{
				printf("next is %lld",current_animal->next_node->akey);
			}
		//	system("pause");
		}
		//this check indicates that
		current_animal->type++; 
		current_animal->current_pro_id++;
		 //adding the next animal to farm pointer
		 
		
		 if(current_animal->next_node!=NULL)
		 {
		 next_animal = current_animal->next_node ;
		 next_animal->previous_node=NULL;
		 FarmProductionList[i*3+1] = next_animal;
	     }
	     else
	     {
	     	FarmProductionList[i*3+1] = NULL;
		 }
		 current_animal->previous_node = NULL;
		 current_animal->next_node = NULL;
		 
		 //update number of animals in the unit
		 current_status = current_animal->tb_status;
		 FarmProductionStatus[i*3+1][current_status+1]--;
		 FarmProductionStatus[i*3+1][0]--;
		 
		 
		 //adding the current animal to the destination
		  // first animal in adult list
		 if(FarmProductionList[i*3+2]==NULL)
		 {
		 	FarmProductionList[i*3+2] = current_animal;
		 }
		 else
		 {
		 	target_animal = FarmProductionList[i*3+2];
		 	target_animal->previous_node = current_animal;
		 	FarmProductionList[i*3+2] = current_animal;
		 	current_animal->next_node = target_animal;
		 }
		 
     	 FarmProductionStatus[i*3+2][current_status+1]++;
		 FarmProductionStatus[i*3+2][0]++;
		 
	    }//while ends
  //   	printf("heifer ends");
     	//next calf
     	while(FarmProductionList[i*3]!=NULL)//if there are heifer
     	{
     		
     	//current_animal = (struct animal_node*)malloc(sizeof( struct animal_node )); 
     	//next_animal = (struct animal_node*)malloc(sizeof( struct animal_node ));
     	//target_animal = (struct animal_node*)malloc(sizeof( struct animal_node ));
     	
		 current_animal = FarmProductionList[i*3]; //get the first animal
		 //if checking age, do here in future
		current_animal->type++;
		current_animal->current_pro_id++; 
		if(current_animal->akey==2454078)
		{
			printf("calf unit is %lld i is %d", current_animal->current_pro_id, i);
			//printf("this animal is now farm %lld and previous animal is %lld next is %lld",current_animal->current_pro_id,current_animal->previous_node->akey,current_animal->next_node->akey);
			if(current_animal->next_node!=NULL)
			{
				printf("next is %lld",current_animal->next_node->akey);
			}
			//system("pause");
		}
		 //adding the next animal to farm pointer
		 
		 if(current_animal->next_node!=NULL)
		 {
		 	next_animal = current_animal->next_node ;
		 next_animal->previous_node=NULL;
		 FarmProductionList[i*3] = next_animal;
	     }
	     else
	     {
	     	FarmProductionList[i*3] = NULL ;
		 }
		 current_animal->previous_node = NULL;
		 current_animal->next_node = NULL;
		 
		 //update number of animals in the unit
		 current_status = current_animal->tb_status;
		 FarmProductionStatus[i*3][current_status+1]--;
		 FarmProductionStatus[i*3][0]--;
		 
		 
		 //adding the current animal to the destination
		  // first animal in adult list
		 if(FarmProductionList[i*3+1]==NULL)
		 {
		 	FarmProductionList[i*3+1] = current_animal;
		 }
		 else
		 {
		 	target_animal = FarmProductionList[i*3+1];
		 	target_animal->previous_node = current_animal;
		 	FarmProductionList[i*3+1] = current_animal;
		 	current_animal->next_node = target_animal;
		 }
		 
     	 FarmProductionStatus[i*3+1][current_status+1]++;
		 FarmProductionStatus[i*3+1][0]++;
		 
	    }//calf ends
//	    printf("calf ends %d",i);
	 }  	
	
//printf("now free memory");
  //       free(current_animal);
    //     free(target_animal);
    //     free(next_animal);
printf("ends");	
}




/*----------------------------------------------------------------------------------*/
/* CALCULATE NEXT MARKOV EVENT DATE*/
/*----------------------------------------------------------------------------------*/
int update_markov_date(int today_date, long long **FarmData, double **FarmProductionStatus,struct animal_node *FarmProductionList[],int num_farm_production,double beta_a,double beta_b, int next_non_markov_date, struct event_node *event_day[])
{

double inf_pressure, inf_pressure_wild,sum_inf_pressure,cumu_pressure;
sum_inf_pressure = 0;
cumu_pressure = 0;
double day_to_markov;
int farm_id ;
int k = 0;

 for(i=0; i<num_farm_production; i++)
	{//for loop A
	
	farm_id = (int)floor(i/3) ;
	if ((FarmData[farm_id][3]==0||FarmProductionStatus[i][3]>0||FarmProductionStatus[i][4]>0)&&FarmProductionStatus[i][1]>0&&FarmProductionStatus[i][0])
	{
		//if(FarmProductionStatus[i][0]<0)
		//{
		//	printf("N is <0 something wrong!");
		//}
		//printf("yes this farm is %d",i);
	    if(FarmData[farm_id][3]==0) //if it is in MCA
	    {
	    	
	      inf_pressure_wild = beta_b*FarmProductionStatus[i][1] ;
	    //  printf("inf pressure wild is %f",inf_pressure_wild);
	      
		}
		else
		{
		inf_pressure_wild = 0;	
		}
		inf_pressure = beta_a*FarmProductionStatus[i][1]*(FarmProductionStatus[i][3]+FarmProductionStatus[i][4])/FarmProductionStatus[i][0] + inf_pressure_wild ; // bSI/N + wildlife
		FarmProductionStatus[i][5] = inf_pressure;
		cumu_pressure = cumu_pressure + inf_pressure; // calculate the cumulative pressure
		FarmProductionStatus[i][6] = cumu_pressure;
		sum_inf_pressure = sum_inf_pressure + inf_pressure;	
		//printf("inf pressure is %f",sum_inf_pressure);
	}
	
	}//END OF for loop A	
	//printf("inf pressure is %f",sum_inf_pressure);
	 double random_value = ((double)rand()/(double)RAND_MAX) ;
	 //printf("random value is %f",random_value);
     day_to_markov =(-log(random_value))/sum_inf_pressure ; // Waiting time
     //printf("day_to_markov is %f",day_to_markov);
//ASSESS IF NEXT MARKOV TIME IS BEFORE NEXT NON MARKOV TIME     
	if (next_non_markov_date>day_to_markov+today_date)
	{ // if markov comes first, choose infection events
	   k++;
	   //printf("This is %d th markov",k);
	   today_date = ceil(day_to_markov+today_date) ;
	   //printf("ceil days is %d",today_date);
	   //system("pause");
	   double random_value = (double)rand()/(double)(RAND_MAX)*sum_inf_pressure;
	   //printf("random_value is %f",random_value);
	   
	   double current_value = 0;
	   int pro_id = 0;
	   
	   while(random_value>current_value)
	   {
	   	farm_id = (int)floor(pro_id/3) ;
	   	if ((FarmData[farm_id][3]==0||FarmProductionStatus[pro_id][3]>0||FarmProductionStatus[pro_id][4]>0)&&FarmProductionStatus[pro_id][0])
	       {
	       	current_value = FarmProductionStatus[pro_id][6] ;
	     //  	printf("current_value is %f",current_value);
	      // 	printf("random value is %f",random_value);
	       if(current_value>=random_value)
	       {
	       	break;
		   }
	       	pro_id++;
		   }
		else
		   {
		   	pro_id++;
		   //	printf("next pro id");
		   }
	   
	   }//pro_id is the farm to choose for the event
	   //now get the first animal in the production unit
	   struct animal_node *current_animal;
	   //current_animal = (struct animal_node*)malloc(sizeof( struct animal_node ));
	   current_animal = FarmProductionList[pro_id];
	   int total_s = FarmProductionStatus[pro_id][1];
	   while(current_animal!=NULL)
	    {//choosing which animal to infect begins
	   	double random_value = ((double)rand()/(double)RAND_MAX); //choose value between 0 and 1
	   	if (random_value <= 1/total_s)
	   	 {
	   	 	//printf("random value is %f",random_value);
	   	 	//printf("target value is %f",1/total_s);
	   	 	printf("previous status %d",current_animal->tb_status);
	   	 	current_animal->tb_status = 1;
	   	 	printf("new infection");
	   	 	FarmProductionStatus[pro_id][1]--;
	   	 	FarmProductionStatus[pro_id][2]++;
	   	 	//need to set it's event to be occult
	   	 	struct event_node *adding_new_event;
	   	 	int day_to_occult = rand()%max_day_occult+1;
		      int day_to_add = day_to_occult + today_date;
			  
		      if(day_to_add<sim_days)
			  {
		      adding_new_event = (struct event_node*)malloc(sizeof( struct event_node ));
		      adding_new_event->akey=current_animal->akey;
		      adding_new_event->src_pro_id = -100; 
		      adding_new_event->des_pro_id = -100;
		      adding_new_event->src_testarea=-100;
			  adding_new_event->event_type=6;//occult to detectable happens
			  
			  add_event_node(event_day, day_to_add, adding_new_event) ;
		      }
	   	 	
	   	 	break ;
		 }
		else
		 {
		    current_animal = current_animal->next_node;	
		   // printf("next animal");
		 }
	    } //choosing which animal to infect done  
	}
	else
	{
		today_date = next_non_markov_date;
	}
//ASSESSING MARKOV OR NOT DONE	
return(today_date) ;
    
    
}//END OF FUNCTION
/*------------------------------------------------------------------------------------------------*/
/*COUNT NUMBER OF INFECTED AND DETECTED FARMS*/
/*--------------------------------------------------------------*/
void count_farms_infected_detected(long long **FarmData, double **OutPut, long num_total_farms, int sim)
{
long i;
for (i=0;i<num_total_farms;i++)
{
	if(FarmData[i][7]>0||FarmData[i][8]>0)
	{
	 OutPut[sim][0]++; //infected	
	 //printf("one more infected");
	}
	if(FarmData[i][6]==1)
	{
	 OutPut[sim][1]++;//detected
	// printf("one more detected");
	}
}
 	
	
	
}


/*-----------------------------------------------------------------------------*/
/*Export CSV file of the number of bTB detected\infected data*/
/*------------------------------------------------------------------------------*/
int write_OutPut(char OutDataFile[],double **OutPut, int tot_iterations, int num_OutPut)
{

	FILE *Out = fopen(OutDataFile,"w");
	int line_num, col_num;
	
	for (line_num = 0 ; line_num < tot_iterations; line_num ++)
	{
		for (col_num = 0;col_num <num_OutPut ; col_num++ )
		
		{
	    fprintf(Out,"%lf,",OutPut[line_num][col_num]);
        }
        fprintf(Out,"\n");
    }
	fclose(Out);
	return 0;
}
