Parameters: simulation_time, probability, snapshot time t, seed

Usage:    g++ -pthread -o sim sim.cc

              ./sim 60 0.4 1 35

Mutexes: creation mutex for changing size of the lane queues
               police mutex for changing honk number

Condition variable: Police will wait until a honk event.
In our case, 0.4 is not appropriate for this event. Try with 0.1.

policeChoice is a variable which helps the police decide which lane will be on the current flow. Unless given two conditions are satisfied, the current flowing lane will keep flowing. In this case, the policeChoice isn't changed and transferred to the following iteration unchanged. If any lane has more than or equal to 5 cars, policeChoice is changed to 0 (indecisive) for the police to check the lane with the maximum cars. If any car is waiting for more than 20 seconds, policeChoice is changed to that lane number and next flow will start from that number. Briefly, policeChoice is the decision for the lane number. If it is equal to 0, police has to decide immediately in that iteration based on the lane with maximum number of cars.