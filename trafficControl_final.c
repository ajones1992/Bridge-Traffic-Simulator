/*
AUTHOR: Alex Jones
PROGRAM: Bridge Traffic Control
GOAL: Run batch traffic across the bridge
*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#define BRIDGE_LIMIT 12

// Type of vehicle, car or van
typedef enum
{
    CAR,
    VAN
} v_type;

// Direction of vehicle, north or south
typedef enum
{
    SOUTH,
    NORTH
} v_dir;

// Used to pass vehicle attributes to the thread routine, id, type, direction
typedef struct
{
    int idT;
    v_type type;
    v_dir dir;
} vehicle_init;

int th_quant; // Used to ensure threads complete cross() before program terminates
int l1_count = 0; // Bridge lane one traffic tracker
int l2_count = 0; // Bridge lane two traffic tracker
int bridge_curr_weight = 0; // Weight on the bridge according to the vehicles currently involved
int v_completed = 0; // Completed vehicle count
v_dir l1; // Bridge lane one direction
v_dir l2; // Bridge lane two direction
pthread_mutex_t bLock = PTHREAD_MUTEX_INITIALIZER; // Lock to be used in critical sections
pthread_cond_t bCon = PTHREAD_COND_INITIALIZER; // Condition variable to be used with the lock

int arrive(v_type cv, v_dir vdir, int tid)
{
    /*
     * Function goal: Check bridge restrictions (direction, weight), print message, return.
     *                Restrictions: Cars weigh 2, vans 3, bridge cannot exceed 12. 2 lanes, 
     *                              one for each direction or both for one direction if both 
     *                              are empty at the start.
     */

    int wgt = (cv == CAR) ? 2 : 3; // Determine the weight of the vehicle
    char ca_v[4]; // Variable for printing vehicle type
    cv == CAR ? strcpy(ca_v, "Car") : strcpy(ca_v, "Van"); // Determine ca_v value
    char aDir[6]; // Variable for printing vehicle direction
    vdir == NORTH ? strcpy(aDir, "North") : strcpy(aDir, "South"); // Determine vDir value
    printf("%s #%d (%sbound) has arrived. \n", ca_v, tid, aDir); // Print arrival message
    char l1_d[6]; // String representation of lane one direction
    char l2_d[6]; // String representation of lane two direction

    // Obtain the lock
    while (pthread_mutex_trylock(&bLock) != 0)
    {
        sleep(1);
    }

    // If bridge is too heavy, wait until it has less weight
    while (bridge_curr_weight + wgt > BRIDGE_LIMIT)
    {
        pthread_cond_wait(&bCon, &bLock);
    }

    // Determine which lane the vehicle will take, and add one to that lane count
    if (l1 == vdir)
    {
        l1_count++;
    }
    else if (l2 == vdir)
    {
        l2_count++;
    }
    else
    {
        while (l1_count > 0 && l2_count > 0)
        {
            pthread_cond_wait(&bCon, &bLock);
        }
        if (l1_count == 0)
        {
            l1 = vdir;
            l1_count++;
        }
        else
        {
            l2 = vdir;
            l2_count++;
        }
    }

    // Add weight to the bridge, periodically print out bridge status
    bridge_curr_weight += wgt;
    l1 == NORTH ? strcpy(l1_d, "north") : strcpy(l1_d, "south");
    l2 == NORTH ? strcpy(l2_d, "north") : strcpy(l2_d, "south");
    if (bridge_curr_weight == 12 || bridge_curr_weight == 6)
    {
        printf("Lane 1 direction and quantity: %sbound "
               "with %d vehicles\n"
               "Lane 2 direction and quantity: %sbound "
               "with %d vehicles\n"
               "%d vehicles still in transit.\n",
                l1_d, l1_count, l2_d, l2_count, v_completed);
    }

    // Give up the lock and return
    pthread_mutex_unlock(&bLock);
    return (0);
}

int cross(v_type cv, int tid)
{
    /*
     * Function goal: Print message, induce 3 second delay, return.
     */

    char cc_v[4]; // Variable for printing vehicle type
    cv == CAR ? strcpy(cc_v, "Car") : strcpy(cc_v, "Van"); // Determine cc_v value
    printf("%s #%d is crossing.\n", cc_v, tid); // Print message
    sleep(3); // 3 second delay (th_quant ensures success of this call)
    return (0);
}

int leave(v_type cv, v_dir vdir, int tid)
{
    /*
     * Function goal: Print message, update internal data to upkeep traffic flow.
     */
    int wgt = cv == VAN ? 3 : 2; // Determine vehicle weight (redundant call, could be improved)
    char cl_v[4]; // Variable for printing vehicle type
    cv == CAR ? strcpy(cl_v, "Car") : strcpy(cl_v, "Van"); // Determine cl_v value
    printf("%s #%d is leaving.\n", cl_v, tid); // Print message

    // Obtain lock
    while (pthread_mutex_trylock(&bLock) != 0)
    {
        sleep(1);
    }

    // Take weight off bridge
    bridge_curr_weight -= wgt;

    // Decrement count on the lane
    if (l1 == l2)
    {
        if (l1_count >= l2_count)
        {
            l1_count--;
        }
        else
        {
            l2_count--;
        }
    }
    else if (l1 == vdir)
    {
        l1_count--;
    }
    else
    {
        l2_count--;
    }

    // Decrement th_quant to signal thread is done
    th_quant--;

    // Decrement v_completed for bridge status printout
    v_completed--;

    // Give up lock, signal waiting thread, and return
    pthread_mutex_unlock(&bLock);
    pthread_cond_signal(&bCon);
    return (0);
}

void *vehicleRoutine(void *input)
{
    /*
     * Function goal: Call functions arrive(), cross(), leave()
     */

    vehicle_init *arguments = input; // Initialize inputs
    
    // Call arrive with the input type, direction, and id
    arrive(arguments->type, arguments->dir, arguments->idT);

    // Call cross with the input type and id
    cross(arguments->type, arguments->idT);

    // Call leave with the input type, direction, and id
    leave(arguments->type, arguments->dir, arguments->idT);

    // Free the memory used for the input. This ensures proper information passing from thread creation.
    free(input);

    // Return
    return (0);
}

int main()
{
    /*
     * Function goal: Take user input for quantity of groups of vehicles, quantity of vehicles,
     *                probability of direction, and delays between groups.
     */

    int groups; // Amount of groups to be simulated
    int i, j; // Loop indexes    
    printf("Enter # of groups: \n"); // Prompt user for group amount
    scanf("%d", &groups); // Scan input into groups variable

    // Initialize the thread, direction probability, and delay time arrays with the group size
    int vehicleQ[groups];
    float northP[groups];
    int delayL[groups];

    // For each group, obtain the amount of vehicles (threads), direction probability, and delay
    for (i = 0; i < groups; i++)
    {
        printf("Enter quantity of vehicles: \n");
        scanf("%d", &vehicleQ[i]);
        printf("Enter North probability <=100: \n");
        scanf("%3f", &northP[i]);
        printf("Enter delay in seconds: \n");
        scanf("%d", &delayL[i]);
    }
    
    // For each group, simulate the bridge
    for (i = 0; i < groups; i++)
    {
        v_completed = vehicleQ[i]; // Reset vehicle counter
        pthread_t vehicles[vehicleQ[i]]; // Initialize thread array
        printf("Group #%d simulating now.\n", i + 1); // Print out for user clarification

        for (j = 0; j < vehicleQ[i]; j++) // Loop vehicle quantity
        {
            th_quant++; // Ensures all threads complete before delay is called
            vehicle_init *t_attr = malloc(sizeof(*t_attr)); // Vehicle attribute struct for thread creation

            srand(time(NULL) + j); // Seed the random number generator
            float probability = (float)rand() / RAND_MAX; // Generate a random probability between 0 and 1
            
            if (probability <= 0.5) // CAR if probability is .5 or less
            {
                t_attr->type = CAR;
            }
            else // VAN if greater than .5
            {
                t_attr->type = VAN;
            }

            if (northP[i] == 100) // If all vehicles should go north
            {
                t_attr->dir = NORTH;
            }
            else if (northP[i] == 0) // If all vehicles should go south
            {
                t_attr->dir = SOUTH;
            }
            else // If user entered 1-99, determine vehicle direction 
            {
                probability = (float)rand() / RAND_MAX; // Generate new number between 0 and 1
                if (probability <= (northP[i] / 100.0)) // NORTH if probability is less or equal to given input
                {
                    t_attr->dir = NORTH;
                }
                else // SOUTH if probability is greater than input
                {
                    t_attr->dir = SOUTH;
                }
            }

            t_attr->idT = j + 1; // Vehicle id
            pthread_create(&vehicles[j], NULL, &vehicleRoutine, t_attr); // Create the thread
        }

        // Ensure all threads complete their tasks
        while (th_quant > 0)
        {
            sleep(1);
        }

        printf("Interim delay for group #%d commencing.\n", i+1);
        sleep(delayL[i]); // Delay between groups
    }

    printf("All groups successfully simulated.\n"); // Printout for user clarification
    exit(0); // Program complete
}
