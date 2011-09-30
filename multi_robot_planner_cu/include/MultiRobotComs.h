/* ----------------------- threading and socket stuff ------------------ */

class GlobalVariables
{
  public:

   GlobalVariables(); 
   GlobalVariables(int num_of_agents);
   ~GlobalVariables();
   void Populate(int num_of_agents);
   void Reset();                       // resets for a new planning cycle   

   bool set_up_agent_address(int ag_id, const char* IP_string);

   bool have_all_team_start_and_goal_data();
   bool all_team_ready_to_plan();

   void broadcast(void* buffer, size_t buffer_size); // sends data to all robots we have info from
   void hard_broadcast(void* buffer, size_t buffer_size); // sends data to all robots we think may exist, regardless of if we have info from them
   
   int populate_buffer_with_all_robot_data(char* buffer); // puts data about all robots the sender knows about into the buffer, returns the number of chars that it required

   bool recover_all_robot_data_from_buffer(char* buffer, int &index, int& sender_id); // gets robot data out of the buffer, updates index, returns true if the planning iteration changes due to what was in the buffer

   float calculate_time_left_for_planning();  // based on info from all agents, this returns the time that remains for planning

   bool JoinedTeams();                        // check if this team needs to be actively joined with another (plans conflict), if so then joins them and returns true

   bool have_all_team_single_paths();         // returns true if we have all team members current single paths, else false

   bool old_team_disolved(const vector<bool> & OldInTeam); // checks to see if the old team has been disolved

   void output_state_data();

   vector<int> InPorts;      // indexed using global ID (i.e. agent number)
   vector<int> OutPorts;     // indexed using global ID (i.e. agent number)
   int MasterInPort;
   int MasterOutPort;

   vector<bool> InTeam;       // InTeam[n] = true means that agent n is in this agent's dynamic team, indexed using global ID (i.e. agent number)
   vector<int> local_ID;      // local_ID[n] gives the index associated with robot n on this agent (i.e. 1 level of id abstraction)
   vector<int> global_ID;     // inverse mapping of local_ID
   
   vector<int> planning_iteration;   // keeps track of how many times each agent has restarted planning, indexed using global ID (i.e. agent number)
  
   vector<int> nav_state;            // keeps track of where each agent is in the bavigation state machine, indexed using global ID (i.e. agent number)
   vector<int> nav_state_iteration;  // keeps track of nav_state, indexed using global ID (i.e. agent number)

   vector<vector<float> > last_known_pose; // indexed using global ID, holds the last known pose of each robot
   vector<int> pose_iteration;  // indexed using global ID, gets incrimented by the robot who's pose is updated used with last_known_pose 

   vector<vector<float> > sub_start_coords;  // sub_start_coords[i] holds the sub-start location for robot i, indexed using Global_ID
   vector<vector<float> > sub_goal_coords;   // sub_goal_coords[i] holds the sub-goal location for robot i, indexed using Global_ID
   vector<int> sub_start_and_goal_iteration; // used with sub_start_coords and sub_goal_coords, gets incrimented when they change

   bool found_single_robot_solution;             // starts false, turns true when single_robot_solution is found
   vector<vector<float> > single_robot_solution; // a single robot solution for this robot from its start to goal, where points are [x y time angle], 
                                                 // and x and y are in global coords
                                                 // always updated to incoporate new multi-robot path stuff and current time/robot location
   vector<vector<vector<float> > > other_robots_single_solutions; //  other_robots_single_solutions[i] holds the single robot solution for robot i
                                                                  //  where i is indexed by global ID (i.e. agent number). 
   vector<int> planning_iteration_single_solutions; // holds the associated planning iteration of other_robots_single_solutions 


   vector<float> last_known_dist;    // last known distance between this robot and the other robots (global index)
   vector<clock_t> last_known_time;  // the time that last_known_dist was captured (global index)
   

   vector<int> have_info;     // have_info[i] gets set to 1 when we get agent i's info (start and goal), indexed using local_ID
   vector<int> agent_ready;   // agent_ready[i] is set to 1 when agent i has enough info to start planning, indexed using local_ID
   vector<int> agent_moving;  // agent_moving[i] set to 1 when i starts moving, indexed using local_ID
   vector<struct sockaddr_in> other_addresses;  // indexed using global ID (i.e. agent number)
   char** other_IP_strings;                     // indexed using global ID (i.e. agent number)
   
   vector<vector<float> > start_coords;  // start_coords[i] holds the start location for robot i, indexed using local_ID
   vector<vector<float> > goal_coords;   // goal_coords[i] holds the goal location for robot i, indexed using local_ID
   
  
   vector<float> planning_time_remaining; // holds the ammount of planning time remaining for each agent, indexed using local_ID
   vector<timeval> last_update_time;      // last_update_time[i] holds the last time planning_time_remaining[i] was updated, indexed using local_ID
   timeval start_time_of_planning;
   float min_clock_to_plan;

   vector<timeval> last_path_conflict_check_time; // holds the last time we checked if this agent conflicts with each other agent

   int agent_number;    // this agent's global id
     
   char master_IP[256]; // only used in structured mode (i.e. not ad-hoc)
   char base_IP[256];   // only used in ad-hoc, all ip addresses are then found as base_IP.(agent_id+1)
   char my_IP[256];
   int  my_out_sock;
   int  my_in_sock;
   
   int number_of_agents;      // total number of agents 
   int min_team_size;         // the dynamic team must be this big to start planning
   int team_size;             // the dynamic team currently has this many members
   
   bool kill_master;          // if true then we shut down all threads
   bool master_reset;         // if true then we restart the planning
   bool done_planning;             // true when done_planning (i.e. moving)
   
   float sync_message_wait_time;  // time to wait between sending messages during sync phases
   float message_wait_time;       // time to wait between sending messages during planning
   
   float robot_radius;
   
   float prob_at_goal;        // probability a new edge goes at the goal
   float move_max;            // max move allowed during rrt generation
   float theta_max;           // max rotation allowed durring rrt generation
   float resolution;          // resolution of the rrt
   float angular_resolution;  // angular resolution of the rrt
   float planning_border_width; // this much more space provided around robots for planning
   
   vector<float> team_bound_area_min;  // holds the minimum point in the hyper cube team bounding area  (note, this gets populated when NavSceen is populated)
   vector<float> team_bound_area_size; // holds the distance along each dimension of the hyper cube team bounding area (note, this gets populated when NavSceen is populated) 
   
   POSE* robot_pose;  // most recent pose of the robot
   
   float path_conflict_combine_dist; // if paths intetsect, then we must be this close to the robot of the other path to join their team
   float combine_dist;   // also combine if robots are this close, even if their paths do not intersect
   float drop_dist;      // after we know a robot is this far away from us, we can drop them from our team (note: combine_dist < drop_dist)
   float drop_time;      // after this long without hearing from a robot we drop it from the team
           
   clock_t start_time; // this gets set at the beginning and then never changed
   void* MAgSln;   // pointer to the multi agent solution

  
   bool have_calculated_start_and_goal;  // gets set to true when we calculate this agent's start and goal



   bool use_sub_sg; // gets set to true if we need to use sub_start and sub_goal instead of start and goal


   bool revert_to_single_robot_path;       // set to true if no team's single robot paths conflict

   float default_map_x_size;  // used only when finding single robot path
   float default_map_y_size;  // used only when finding single robot path

   bool sender_Ad_Hoc_running;  // true when the sender ad hoc thread is running, used to avoid memory problems on reset
   bool listener_active;        // true when the listener thread is doing stuff, used to avoid memory problems on reset
};
