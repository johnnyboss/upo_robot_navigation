#include <upo_rrt_planners/steering/Steering.h>

upo_RRT::Steering::Steering() {
	
	space_ = NULL;
	
	maxRange_ = 0.25;
	
	timeStep_ = 0.1; 
	minControlSteps_ = 5; //minTime = timeStep*minControlSteps
	maxControlSteps_ = 10;
	
	maxLinearAcc_ = 1.0;   // m/s²
	maxAngularAcc_ = 2.0; // rad/s²
	
	max_lv_var_ = maxLinearAcc_ * timeStep_;
	max_av_var_ = maxAngularAcc_ * timeStep_;
	
	steeringType_ = 2;
	motionCostType_ = 2;
}


upo_RRT::Steering::Steering(StateSpace* sp) {
	
	space_ = sp;
	
	maxRange_ = 0.25;
	
	timeStep_ = 0.1; 
	minControlSteps_ = 5; //minTime = timeStep*minControlSteps
	maxControlSteps_ = 10;
	
	maxLinearAcc_ = 1.0;   // m/s²
	maxAngularAcc_ = 2.0; // rad/s²
	
	max_lv_var_ = maxLinearAcc_ * timeStep_;
	max_av_var_ = maxAngularAcc_ * timeStep_;
	
	steeringType_ = 2;
	motionCostType_ = 2;
}


upo_RRT::Steering::Steering(StateSpace* sp, float max_range) 
	: maxRange_(max_range) {
	
	space_ = sp;
	
	timeStep_ = 0.1; 
	minControlSteps_ = 5; //minTime = timeStep*minControlSteps
	maxControlSteps_ = 10;
	
	maxLinearAcc_ = 1.0;   // m/s²
	maxAngularAcc_ = 2.0; // rad/s²
	
	max_lv_var_ = maxLinearAcc_ * timeStep_;
	max_av_var_ = maxAngularAcc_ * timeStep_;
	
	steeringType_ = 2;
	motionCostType_ = 2;
}


upo_RRT::Steering::Steering(StateSpace* sp, float tstep, int minSteps, int maxSteps, float lAccMax, float aAccMax) 
	: timeStep_(tstep),
	minControlSteps_(minSteps), maxControlSteps_(maxSteps), 
	maxLinearAcc_(lAccMax), maxAngularAcc_(aAccMax) {
		
	space_ = sp;	
		
	maxRange_ = 0.0;
	max_lv_var_ = maxLinearAcc_ * timeStep_;
	max_av_var_ = maxAngularAcc_ * timeStep_;
	
	steeringType_ = 2;
	motionCostType_ = 2;
}	

upo_RRT::Steering::Steering(StateSpace* sp, float max_range, float tstep, int minSteps, int maxSteps, float lAccMax, float aAccMax) 
	: space_(sp), maxRange_(max_range), timeStep_(tstep),
	minControlSteps_(minSteps), maxControlSteps_(maxSteps), 
	maxLinearAcc_(lAccMax), maxAngularAcc_(aAccMax) {
		
	max_lv_var_ = maxLinearAcc_ * timeStep_;
	max_av_var_ = maxAngularAcc_ * timeStep_;
	
	steeringType_ = 2;
	motionCostType_ = 2;
}



upo_RRT::Steering::~Steering() {
	//delete space_;
}



upo_RRT::State* upo_RRT::Steering::simpleSteer(State* fromState, State* toState, std::vector<State>& istates)
{
	//State* newState = NULL;

	//Distance between points
	float dx = (toState->getX()-fromState->getX());
	float dy = (toState->getY()-fromState->getY());
	float dt = atan2(dy, dx);
	float dist = sqrt(dx*dx+dy*dy);
	
	float res = space_->getXYresolution();
	unsigned int steps;
	if(dist >= maxRange_)
		steps = (int) floor(maxRange_/res + 0.5);
	else
		steps = (int) floor(dist/res + 0.5);

	//printf("Double distance: %.2f, steps: %u, res: %.2f\n", dist, steps, res); 
	
	State* aux = new State(fromState->getX(), fromState->getY());
	for(unsigned int i=0; i<steps; i++)
	{
		float newx = aux->getX() + res*cos(dt);
		float newy = aux->getY() + res*sin(dt);
		delete aux;
		aux = new State(newx, newy);
		if(space_->isStateValid(aux)) {
			//if(newState)
			//	delete newState;
			//newState = new State(newx, newy);
			//istates.push_back(*newState);
			istates.push_back(*aux);
		} else {
			delete aux;
			aux = NULL;
			break;
		}
	}
	if(aux)
		delete aux;

	State* newState = NULL;
	if(istates.size() > 0)
		newState = &(istates.at(istates.size()-1));
		
	return newState;
}


bool upo_RRT::Steering::simpleCollisionFree(State* fromState, State* toState, std::vector<State>& istates)
{

	//Distance between points
	float dx = (toState->getX()-fromState->getX());
	float dy = (toState->getY()-fromState->getY());
	float dt = atan2(dy, dx);
	float dist = sqrt(dx*dx+dy*dy);

	float res = space_->getXYresolution();
	unsigned int steps = (int) floor(dist/res + 0.5);
	
	//Check the validity of the steps of the line from the initial state
	//to the max_range distance  
	State* aux = new State(fromState->getX(), fromState->getY());
	for(unsigned int i=0; i<steps; i++)
	{
		float newx = aux->getX() + cos(dt)*res;
		float newy = aux->getY() + sin(dt)*res;
		delete aux;
		aux = new State(newx, newy);
		if(!space_->isStateValid(aux)) {
			delete aux;
			return false;
		}
		istates.push_back(*aux);
	}
	if(aux)
		delete aux;
		
	return true;
}


//Steering used in KinoRRT (only 2 dimensions, and one action between states)
bool upo_RRT::Steering::rrt_steer(Node* fromNode, Node* toNode, Node* newNode)
{
	std::vector<State> istates;
	
	if(fromNode == NULL) {
		printf("Steering. Nodo inicial igual a NULL\n"); 
		return false;
	}
	if(toNode == NULL) {
		printf("Steering. Nodo final igual a NULL\n"); 
		return false;
	}	
	
	max_lv_var_ = maxLinearAcc_ * timeStep_;
	max_av_var_ = maxAngularAcc_ * timeStep_;
	
	
	//Robot position
	float rx = fromNode->getState()->getX();
	float ry = fromNode->getState()->getY();
	float rth = fromNode->getState()->getYaw();
	//float r_lv = fromNode->getAction()->getVx();
	//float r_av = fromNode->getAction()->getVth();
	
	//waypoint to reach
	float wx = toNode->getState()->getX();
	float wy = toNode->getState()->getY();
	float wth = toNode->getState()->getYaw();
	
	//printf("xr:%.2f, yr:%.2f, xw:%.2f, yw:%.2f\n", rx, ry, wx, wy); 
	
	// Transform way-point into local robot frame and get desired x,y,theta
	float dx = (wx-rx)*cos(rth) + (wy-ry)*sin(rth);
	float dy =-(wx-rx)*sin(rth) + (wy-ry)*cos(rth);
	float dist = sqrt(dx*dx + dy*dy);
	float dt = atan2(dy, dx);
	
	
	//Velocities to command
	float lv = space_->getMaxLinVel() * exp(-fabs(dt))* tanh(3*dist);
	float av = space_->getMaxAngVel() * dt;
	
	float prev_lv = fromNode->getState()->getLinVel();
	float prev_av = fromNode->getState()->getAngVel();
	// linear vel
	if(fabs(prev_lv - lv) > max_lv_var_) {
		if(lv < prev_lv)
				lv = prev_lv - max_lv_var_;
			else
				lv = prev_lv + max_lv_var_;
	} 
	// angular vel
	if(fabs(prev_av - av) > max_av_var_) {
		if(av < prev_av)
				av = prev_av - max_av_var_;
			else
				av = prev_av + max_av_var_;
	} 
	
	if(lv > space_->getMaxLinVel())
		lv = space_->getMaxLinVel();
	else if(lv < space_->getMinLinVel())
		lv = space_->getMinLinVel();
	
	if(av > space_->getMaxAngVel())
		av = space_->getMaxAngVel();
	else if(av < (-space_->getMaxAngVel()))
		av = space_->getMaxAngVel()*(-1);
		
	//Dead areas
	/*if(fabs(lv) < 0.08)
		lv = 0.0;
	if(fabs(av) < 0.05)
		av = 0.0;
	*/
	
	
	State currentState = *fromNode->getState();
	
	int numSteps = 0;
	while(numSteps <= maxControlSteps_ && dist > space_->getGoalXYTolerance()) 
	{
			
		State* newState = propagateStep(&currentState, lv, av);
		
		if(!space_->isStateValid(newState)) {
			delete newState;
			break;
		}
		
		currentState = *newState;
		delete newState;
		
		istates.push_back(currentState);
		numSteps++;
		dist = sqrt((wx-currentState.getX())*(wx-currentState.getX()) + (wy-currentState.getY())*(wy-currentState.getY()));
	}
	
	if(numSteps == 0) {
		return false;
	}
	Action action(lv, 0.0, av, numSteps);
	//Node* newNode = new Node(currentState, action); 
	newNode->setState(currentState);
	newNode->addAction(action);
	newNode->setIntermediateStates(istates);
	
	return true;
	
}


bool upo_RRT::Steering::rrt_collisionFree(Node* fromNode, Node* toNode, Node& out)
{
	std::vector<State> istates;
	
	if(fromNode == NULL) {
		printf("Steering. Nodo inicial igual a NULL\n"); 
		return false;
	}
	if(toNode == NULL) {
		printf("Steering. Nodo final igual a NULL\n"); 
		return false;
	}	
	
	max_lv_var_ = maxLinearAcc_ * timeStep_;
	max_av_var_ = maxAngularAcc_ * timeStep_;
	
	
	//Robot position
	float rx = fromNode->getState()->getX();
	float ry = fromNode->getState()->getY();
	float rth = fromNode->getState()->getYaw();
	//float r_lv = fromNode->getAction()->getVx();
	//float r_av = fromNode->getAction()->getVth();
	
	//waypoint to reach
	float wx = toNode->getState()->getX();
	float wy = toNode->getState()->getY();
	float wth = toNode->getState()->getYaw();
	
	//printf("xr:%.2f, yr:%.2f, xw:%.2f, yw:%.2f\n", rx, ry, wx, wy); 
	
	// Transform way-point into local robot frame and get desired x,y,theta
	float dx = (wx-rx)*cos(rth) + (wy-ry)*sin(rth);
	float dy =-(wx-rx)*sin(rth) + (wy-ry)*cos(rth);
	float dist = sqrt(dx*dx + dy*dy);
	float dt = atan2(dy, dx);
	
	
	//Velocities to command
	float lv = space_->getMaxLinVel() * exp(-fabs(dt))* tanh(3*dist);
	float av = space_->getMaxAngVel() * dt;
	
	float prev_lv = fromNode->getState()->getLinVel();
	float prev_av = fromNode->getState()->getAngVel();
	// linear vel
	if(fabs(prev_lv - lv) > max_lv_var_) {
		if(lv < prev_lv)
				lv = prev_lv - max_lv_var_;
			else
				lv = prev_lv + max_lv_var_;
	} 
	// angular vel
	if(fabs(prev_av - av) > max_av_var_) {
		if(av < prev_av)
				av = prev_av - max_av_var_;
			else
				av = prev_av + max_av_var_;
	} 
	
	if(lv > space_->getMaxLinVel())
		lv = space_->getMaxLinVel();
	else if(lv < space_->getMinLinVel())
		lv = space_->getMinLinVel();
	
	if(av > space_->getMaxAngVel())
		av = space_->getMaxAngVel();
	else if(av < (-space_->getMaxAngVel()))
		av = space_->getMaxAngVel()*(-1);
		
	//Dead areas
	/*if(fabs(lv) < 0.08)
		lv = 0.0;
	if(fabs(av) < 0.05)
		av = 0.0;
	*/
	
	
	//printf("printf: dt: %.2f, dist:%.2f, lv: %.2f, av: %.2f \n\n", dt, dist, lv, av);
	
	float max_dist_step = space_->getMaxLinVel() * timeStep_;
	float approx_steps = dist/max_dist_step;
	
	
	State newState = *fromNode->getState();
	
	int numSteps = 0;
	
	while (dist >= space_->getGoalXYTolerance()) {
		
		//Check that the path to waypoint is not too long
		if(numSteps > ceil(approx_steps*2)) { 
			return false;
		}
		
		//newState = *propagateStep(&newState, lv, av);
		State* st = propagateStep(&newState, lv, av);
		
		//Check if the state is valid
		if(!space_->isStateValid(st)) {
			delete st;
			return false;
		}
		
		newState = *st;
		delete st;
		
		//printf("Step: %i. NewState x:%.2f, y:%.2f, th:%.2f\n", numSteps, newState->getX(), newState->getY(), newState->getYaw());
		istates.push_back(newState);
		numSteps++;
		dist = sqrt((wx-newState.getX())*(wx-newState.getX()) + (wy-newState.getY())*(wy-newState.getY()));
	}
	
	if(numSteps == 0) {
		return false;
	}
	
	Action newAction(lv, 0.0, av, numSteps);
	out.setState(newState);
	std::vector<Action> act;
	act.push_back(newAction);
	out.setAction(act);
	out.setIntermediateStates(istates);
	return true;
}






// Steering method in 2 dimensions (x, y) - Interpolation of the motion cost
bool upo_RRT::Steering::steer2(Node* fromNode, Node* toNode, Node* newNode)
{
	//Node * newNode = NULL;
	//Node * prevNode = NULL;
	std::vector<Action> actions;
	std::vector<State> istates;
	
	if(fromNode == NULL) {
		printf("Steering. Nodo inicial igual a NULL\n"); 
		return false;
	}
	if(toNode == NULL) {
		printf("Steering. Nodo final igual a NULL\n"); 
		return false;
	}	
	
	
	//Max velocities variations in one time step
	max_lv_var_ = maxLinearAcc_ * timeStep_;
	max_av_var_ = maxAngularAcc_ * timeStep_;
	
	
	float incCost = 0.0;
	float AccCost = fromNode->getAccCost();
	
	//float prev_lv = act.at(act.size()-1)->getVx();
	//float prev_av = act.at(act.size()-1)->getVth();
	float prev_lv = fromNode->getState()->getLinVel();
	float prev_av = fromNode->getState()->getAngVel();
	
	
	//waypoint to reach
	float wx = toNode->getState()->getX();
	float wy = toNode->getState()->getY();
	float wth = toNode->getState()->getYaw();
	
	//Parameters for the steering function
	float kp = space_->getMaxLinVel();  	// friburg: 1
    float kv = 3.0;     					// friburg: 3.8
    float ka = space_->getMaxAngVel();     	// friburg: 6
    //float ko = ka/8.0;    				// friburg: -1
	
	float lv = 0.0, av=0.0;
	
	float phi = 0.0;
	float dist = 100.0;
	int numSteps = 0;
	
	//Node currentNode = *fromNode;
	State currentState = *fromNode->getState();
	Node currentNode(currentState);
	currentNode.setCost(space_->getCost(currentNode.getState()));
	
	while(numSteps <= maxControlSteps_ && dist > space_->getGoalXYTolerance())
	{
		
		// Transform way-point into local robot frame and get desired x,y,theta
		float dx = (wx-currentState.getX())*cos(currentState.getYaw()) + (wy-currentState.getY())*sin(currentState.getYaw());
		float dy =-(wx-currentState.getX())*sin(currentState.getYaw()) + (wy-currentState.getY())*cos(currentState.getYaw());
		if(numSteps == 0)
			dist = sqrt(dx*dx + dy*dy);  

		float alpha = atan2(dy, dx);
		
		//Astolfi
		//float lv = kp * dist;
		//float av = ka * alpha; // + ko * phi;
		
		if(steeringType_ == 1) {
			//POSQ
			lv = kp * tanh(kv*dist);
			av = ka * alpha; 
		} else {
			//Improved-POSQ
			lv = kp * tanh(kv*dist) * exp(-fabs(alpha));
			av = ka * alpha;
		}
		
		//Check velocities reacheability
		// linear vel
		if(fabs(prev_lv - lv) > max_lv_var_) {
			if(lv < prev_lv)
					lv = prev_lv - max_lv_var_;
				else
					lv = prev_lv + max_lv_var_;
		} 
		// angular vel
		if(fabs(prev_av - av) > max_av_var_) {
			if(av < prev_av)
					av = prev_av - max_av_var_;
				else
					av = prev_av + max_av_var_;
		} 
		
		//Check max and min velocities
		if(lv > space_->getMaxLinVel())
			lv = space_->getMaxLinVel();
		else if(lv < space_->getMinLinVel())
			lv = space_->getMinLinVel();
		
		if(av > space_->getMaxAngVel())
			av = space_->getMaxAngVel();
		else if(av < (-space_->getMaxAngVel()))
			av = space_->getMaxAngVel()*(-1);
			
		//Dead velocity ranges
		//if(fabs(lv) < 0.08)
		//	lv = 0.0;
		//if(fabs(av) < 0.05)
		//	av = 0.0;
		
		//Propagate the movement
		State* st = propagateStep(&currentState, lv, av);
		
		//Break the loop is the state is not valid
		if(!space_->isStateValid(st)) {
			delete st;
			break;
		}
		
		Node nextNode(*st);
		nextNode.setCost(space_->getCost(st));
		float mc = motionCost(&currentNode, &nextNode);
		incCost += mc;
		currentState = *st;
		delete st;
		currentNode = nextNode;
		currentNode.setIncCost(incCost);
		
		dist = sqrt((wx-currentState.getX())*(wx-currentState.getX()) + (wy-currentState.getY())*(wy-currentState.getY()));
		istates.push_back(currentState);
		Action a(lv, 0.0, av, 1);
		actions.push_back(a);
	
		prev_lv = lv;
		prev_av = av;
	
		numSteps++;
	}
	
	if(numSteps == 0){
		return false;
	}
		
	//currentNode.setAction(actions);
	//currentNode.setIntermediateStates(istates);
	//currentNode.setAccCost(AccCost + incCost);
	
	//newNode = new Node(currentState);
	newNode->setState(currentState);
	newNode->setAction(actions);
	newNode->setIntermediateStates(istates);
	newNode->setCost(currentNode.getCost());
	newNode->setIncCost(incCost);
	newNode->setAccCost(AccCost + incCost);
	
	return true;
}





//Steering for collision checking in two dimensions (x, y) - Interpolation of the motion cost
bool upo_RRT::Steering::collisionFree2(Node* fromNode, Node* toNode, std::vector<Action>& acts, std::vector<State>& istates, float& motCost)
{
	std::vector<Action> actions;
	std::vector<State> inter_states;
	
	if(fromNode == NULL) {
		printf("Steering. Nodo inicial igual a NULL\n"); 
		motCost = 0.0;
		return false;
	}
	if(toNode == NULL) {
		printf("Steering. Nodo final igual a NULL\n"); 
		motCost = 0.0;
		return false;
	}	
	
	
	float incCost = 0.0;
	//float AccCost = fromNode->getAccCost();
	
	//Max velocities variations in one time step
	max_lv_var_ = maxLinearAcc_ * timeStep_;
	max_av_var_ = maxAngularAcc_ * timeStep_;
	
	//std::vector<Action*> act = fromNode->getAction();
	//float prev_lv = act.at(act.size()-1)->getVx();
	//float prev_av = act.at(act.size()-1)->getVth();
	float prev_lv = fromNode->getState()->getLinVel();
	float prev_av = fromNode->getState()->getAngVel();
	
	//waypoint to reach
	float wx = toNode->getState()->getX();
	float wy = toNode->getState()->getY();
	float wth = toNode->getState()->getYaw();
	
	
	float kp = space_->getMaxLinVel();  // 1
    float kv = 3.0;     // 3.8
    float ka = space_->getMaxAngVel();     // 6
    //float ko = ka/8.0;    // -1
	
	float lv = 0.0, av=0.0;
	
	float phi = 0.0;
	float xs = wx - fromNode->getState()->getX();
	float ys = wy - fromNode->getState()->getY();
	float init_dist = sqrt(xs*xs + ys*ys);  //10.0;
	float dist = init_dist;
	
	float max_dist_step = space_->getMaxLinVel() * timeStep_;
	float approx_steps = init_dist/max_dist_step;
	
	
	//Initial robot position
	State currentState = *fromNode->getState();
	//Node currentNode(currentState);
	Node currentNode(*fromNode);
	
	
	int numSteps = 0;
	
	while (dist >= space_->getGoalXYTolerance())  
	{
		
		//Check that the path to waypoint is not too long
		if(numSteps > ceil(approx_steps*3)) { 
			//printf("Step number > %.1f\n", ceil(approx_steps*5));
			motCost = 0.0;	
			return false;
		}
		
		// Transform way-point into local robot frame and get desired x,y,theta
		float dx = (wx-currentState.getX())*cos(currentState.getYaw()) + (wy-currentState.getY())*sin(currentState.getYaw());
		float dy =-(wx-currentState.getX())*sin(currentState.getYaw()) + (wy-currentState.getY())*cos(currentState.getYaw());
		if(numSteps == 0)
			dist = sqrt(dx*dx + dy*dy);  

		float alpha = atan2(dy, dx);
		
		
		//Velocities to command
		if(steeringType_ == 1) {
			//POSQ
			lv = kp * tanh(kv*dist);
			av = ka * alpha; // + ko * phi;
		} else {
			//Improved-POSQ
			lv = kp * tanh(kv*dist) * exp(-fabs(alpha));
			av = ka * alpha; // + ko * phi;
		}
		
		
		//Check velocities reacheability
		
		// linear vel
		if(fabs(prev_lv - lv) > max_lv_var_) {
			if(lv < prev_lv)
					lv = prev_lv - max_lv_var_;
				else
					lv = prev_lv + max_lv_var_;
		} 
		// angular vel
		if(fabs(prev_av - av) > max_av_var_) {
			if(av < prev_av)
					av = prev_av - max_av_var_;
				else
					av = prev_av + max_av_var_;
		} 
		
		if(lv > space_->getMaxLinVel())
			lv = space_->getMaxLinVel();
		else if(lv < space_->getMinLinVel())
			lv = space_->getMinLinVel();
		
		if(av > space_->getMaxAngVel())
			av = space_->getMaxAngVel();
		else if(av < (-space_->getMaxAngVel()))
			av = space_->getMaxAngVel()*(-1);
			
		//Dead areas
		/*if(fabs(lv) < 0.08)
			lv = 0.0;
		if(fabs(av) < 0.05)
			av = 0.0;
		*/
		
		State* st = propagateStep(&currentState, lv, av);
		//Check if the state is valid
		if(!space_->isStateValid(st))  {
			motCost = 0.0;
			delete st;
			return false;
		}
		
		Node nextNode(*st);
		nextNode.setCost(space_->getCost(st));
		float mc = motionCost(&currentNode, &nextNode);
		incCost += mc;
		currentState = *st;
		delete st;
		currentNode = nextNode;
		
		numSteps++;
		
		dist = sqrt((wx-currentState.getX())*(wx-currentState.getX()) + (wy-currentState.getY())*(wy-currentState.getY()));
		
		inter_states.push_back(currentState);
		Action a(lv, 0.0, av, 1);
		actions.push_back(a);
		
		prev_lv = lv;
		prev_av = av;
		
	}
	if(numSteps == 0){
		return false;
	}
	
	
	acts = actions;
	motCost = incCost;
	istates = inter_states;
	
	return true;
	
}









float upo_RRT::Steering::motionCost(Node* n1, Node* n2)
{
	
	switch(motionCostType_)
	{
		case 1:
			// avg
			return ((n1->getCost() + n2->getCost()) / 2.0);
			
		case 2:
			// avg * dist
			//State* s12 = n1->getState();
			//State* s22 = n2->getState();
			//float d = sqrt(((n1->getState()->getX() - n2->getState()->getX())*(n1->getState()->getX() - n2->getState()->getX())) 
			//	+ ((n1->getState()->getY() - n2->getState()->getY())*(n1->getState()->getY() - n2->getState()->getY()))); 
			//return (((n1->getCost() + n2->getCost()) / 2.0) * d);
			return (((n1->getCost() + n2->getCost()) / 2.0) * (sqrt(((n1->getState()->getX() - n2->getState()->getX())*(n1->getState()->getX() - n2->getState()->getX())) 
				+ ((n1->getState()->getY() - n2->getState()->getY())*(n1->getState()->getY() - n2->getState()->getY())))));
		
		case 3:
			// avg * exp(dist)
			//State* s13 = n1->getState();
			//State* s23 = n2->getState();
			//float dist3 = sqrt(((s13->getX() - s23->getX())*(s13->getX() - s23->getX())) + ((s13->getY() - s23->getY())*(s13->getY() - s23->getY()))); 
			//return ((n1->getCost() + n2->getCost()) / 2.0) * exp(dist3);
			return (((n1->getCost() + n2->getCost()) / 2.0) * (exp(sqrt(((n1->getState()->getX() - n2->getState()->getX())*(n1->getState()->getX() - n2->getState()->getX())) 
				+ ((n1->getState()->getY() - n2->getState()->getY())*(n1->getState()->getY() - n2->getState()->getY()))))));
				
		
		case 4:
			// sum 
			return (n1->getCost() + n2->getCost());
		
		
		default:
			//State* s1d = n1->getState();
			//State* s2d = n2->getState();
			//float distd = sqrt(((s1d->getX() - s2d->getX())*(s1d->getX() - s2d->getX())) + ((s1d->getY() - s2d->getY())*(s1d->getY() - s2d->getY()))); 
			//return ((n1->getCost() + n2->getCost()) / 2.0) * distd;
			return ((n1->getCost() + n2->getCost()) / 2.0) * (sqrt(((n1->getState()->getX() - n2->getState()->getX())*(n1->getState()->getX() - n2->getState()->getX())) 
				+ ((n1->getState()->getY() - n2->getState()->getY())*(n1->getState()->getY() - n2->getState()->getY()))));
	}
}





// Steering method in 3 dimensions (x, y, yaw) - Interpolation of the motion cost
bool upo_RRT::Steering::steer3(Node* fromNode, Node* toNode, Node* newNode)
{
	//Node * newNode = NULL;
	//Node * prevNode = NULL;
	std::vector<Action> actions;
	std::vector<State> istates;
	
	if(fromNode == NULL) {
		printf("Steering. Nodo inicial igual a NULL\n"); 
		return false;
	}
	if(toNode == NULL) {
		printf("Steering. Nodo final igual a NULL\n"); 
		return false;
	}	
	
	
	//Max velocities variations in one time step
	max_lv_var_ = maxLinearAcc_ * timeStep_;
	max_av_var_ = maxAngularAcc_ * timeStep_;
	
	
	float incCost = 0.0;
	float AccCost = fromNode->getAccCost();
	
	//float prev_lv = act.at(act.size()-1)->getVx();
	//float prev_av = act.at(act.size()-1)->getVth();
	float prev_lv = fromNode->getState()->getLinVel();
	float prev_av = fromNode->getState()->getAngVel();
	
	
	//waypoint to reach
	float wx = toNode->getState()->getX();
	float wy = toNode->getState()->getY();
	float wth = toNode->getState()->getYaw();
	
	//Parameters for the steering function
	float kp = space_->getMaxLinVel();  	// friburg: 1
    float kv = 3.0;     					// friburg: 3.8
    float ka = space_->getMaxAngVel();     	// friburg: 6
    float ko = ka/8.0;    					// friburg: -1
	
	float lv = 0.0, av=0.0;
	
	float phi = 0.0;
	float dist = 100.0;
	int numSteps = 0;
	
	Node currentNode(*fromNode);
	State currentState = *currentNode.getState();
	//Node currentNode(currentState);
	currentNode.setCost(space_->getCost(currentNode.getState()));
	
	
	while(numSteps <= maxControlSteps_ && (dist > space_->getGoalXYTolerance() || fabs(phi) > space_->getGoalTHTolerance()))
	{
		
		// Transform way-point into local robot frame and get desired x,y,theta
		float dx = (wx-currentState.getX())*cos(currentState.getYaw()) + (wy-currentState.getY())*sin(currentState.getYaw());
		float dy =-(wx-currentState.getX())*sin(currentState.getYaw()) + (wy-currentState.getY())*cos(currentState.getYaw());
		if(numSteps == 0) {
			dist = sqrt(dx*dx + dy*dy);  
			phi = currentState.getYaw() - wth;
			//Normalize phi
			phi = space_->normalizeAngle(phi, -M_PI, M_PI);
		}

		float alpha = atan2(dy, dx);
		
		//Astolfi
		//float lv = kp * dist;
		//float av = ka * alpha; // + ko * phi;
		
		if(steeringType_ == 1) {
			//POSQ
			lv = kp * tanh(kv*dist);
			av = ka * alpha + ko * phi;
		} else {
			//Improved-POSQ
			lv = kp * tanh(kv*dist) * exp(-fabs(alpha));
			av = ka * alpha + ko * phi;
		}
		
		//Check velocities reacheability
		// linear vel
		if(fabs(prev_lv - lv) > max_lv_var_) {
			if(lv < prev_lv)
					lv = prev_lv - max_lv_var_;
				else
					lv = prev_lv + max_lv_var_;
		} 
		// angular vel
		if(fabs(prev_av - av) > max_av_var_) {
			if(av < prev_av)
					av = prev_av - max_av_var_;
				else
					av = prev_av + max_av_var_;
		} 
		
		//Check max and min velocities
		if(lv > space_->getMaxLinVel())
			lv = space_->getMaxLinVel();
		else if(lv < space_->getMinLinVel())
			lv = space_->getMinLinVel();
		
		if(av > space_->getMaxAngVel())
			av = space_->getMaxAngVel();
		else if(av < (-space_->getMaxAngVel()))
			av = space_->getMaxAngVel()*(-1);
			
		//Dead velocity ranges
		/*if(fabs(lv) < 0.08)
			lv = 0.0;
		if(fabs(av) < 0.05)
			av = 0.0;
		*/
		
		//Propagate the movement
		State* st = propagateStep(&currentState, lv, av);
		
		//Break the loop is the state is not valid
		if(!space_->isStateValid(st)) {
			delete st;
			break;
		}
		
		Node nextNode(*st);
		nextNode.setCost(space_->getCost(st));
		float mc = motionCost(&currentNode, &nextNode);
		incCost += mc;
		currentState = *st;
		delete st;
		currentNode = nextNode;
		currentNode.setIncCost(incCost);
		
		dist = sqrt((wx-currentState.getX())*(wx-currentState.getX()) + (wy-currentState.getY())*(wy-currentState.getY()));
		phi = currentState.getYaw() - wth;
		//Normalize phi
		phi = space_->normalizeAngle(phi, -M_PI, M_PI);
		istates.push_back(currentState);
		Action a(lv, 0.0, av, 1);
		actions.push_back(a);
	
		prev_lv = lv;
		prev_av = av;
	
		numSteps++;
		
	}
	
	if(numSteps == 0){
		return false;
	}
	
	
	newNode->setState(currentState);
	newNode->setAction(actions);
	newNode->setIntermediateStates(istates);
	newNode->setCost(currentNode.getCost());
	newNode->setIncCost(incCost);
	newNode->setAccCost(AccCost + incCost);
	
	
	return true;
}





//Steering for collision checking in 3 dimensions (x, y, yaw) - Interpolation of the motion cost
bool upo_RRT::Steering::collisionFree3(Node* fromNode, Node* toNode, std::vector<Action>& acts, std::vector<State>& istates, float& motCost)
{
	std::vector<Action> actions;
	std::vector<State> inter_states;
	
	if(fromNode == NULL) {
		printf("Steering. Nodo inicial igual a NULL\n"); 
		motCost = 0.0;
		return false;
	}
	if(toNode == NULL) {
		printf("Steering. Nodo final igual a NULL\n"); 
		motCost = 0.0;
		return false;
	}	
	
	
	float incCost = 0.0;
	//float AccCost = fromNode->getAccCost();
	
	//Max velocities variations in one time step
	max_lv_var_ = maxLinearAcc_ * timeStep_;
	max_av_var_ = maxAngularAcc_ * timeStep_;
	
	//std::vector<Action*> act = fromNode->getAction();
	//float prev_lv = act.at(act.size()-1)->getVx();
	//float prev_av = act.at(act.size()-1)->getVth();
	float prev_lv = fromNode->getState()->getLinVel();
	float prev_av = fromNode->getState()->getAngVel();
	
	//waypoint to reach
	float wx = toNode->getState()->getX();
	float wy = toNode->getState()->getY();
	float wth = toNode->getState()->getYaw();
	
	
	float kp = space_->getMaxLinVel();  
    float kv = 3.0;     
    float ka = space_->getMaxAngVel();     
    float ko = ka/8.0;    
	
	float lv = 0.0, av=0.0;
	
	float phi = 0.0;
	float xs = wx - fromNode->getState()->getX();
	float ys = wy - fromNode->getState()->getY();
	float init_dist = sqrt(xs*xs + ys*ys);  //10.0;
	float dist = init_dist;
	
	float max_dist_step = space_->getMaxLinVel() * timeStep_;
	float approx_steps = init_dist/max_dist_step;
	
	
	//Initial robot position
	Node currentNode(*fromNode);
	State currentState = *currentNode.getState();
	
	int numSteps = 0;
	
	while (dist >= space_->getGoalXYTolerance() || fabs(phi) > space_->getGoalTHTolerance())  
	{
		
		//Check that the path to waypoint is not too long
		if(numSteps > ceil(approx_steps*3)) { 
			//printf("Step number > %.1f\n", ceil(approx_steps*5));
			motCost = 0.0;	
			return false;
		}
		
		
		
		// Transform way-point into local robot frame and get desired x,y,theta
		float dx = (wx-currentState.getX())*cos(currentState.getYaw()) + (wy-currentState.getY())*sin(currentState.getYaw());
		float dy =-(wx-currentState.getX())*sin(currentState.getYaw()) + (wy-currentState.getY())*cos(currentState.getYaw());
		if(numSteps == 0) {
			dist = sqrt(dx*dx + dy*dy);  
			phi = currentState.getYaw() - wth;
			//Normalize phi
			phi = space_->normalizeAngle(phi, -M_PI, M_PI);
		} 

		float alpha = atan2(dy, dx);
		
		
		//Velocities to command
		if(steeringType_ == 1) {
			//POSQ
			lv = kp * tanh(kv*dist);
			av = ka * alpha + ko * phi;
		} else {
			//Improved-POSQ
			lv = kp * tanh(kv*dist) * exp(-fabs(alpha));
			av = ka * alpha + ko * phi;
		}
		
		
		//Check velocities reacheability
		
		// linear vel
		if(fabs(prev_lv - lv) > max_lv_var_) {
			if(lv < prev_lv)
					lv = prev_lv - max_lv_var_;
				else
					lv = prev_lv + max_lv_var_;
		} 
		// angular vel
		if(fabs(prev_av - av) > max_av_var_) {
			if(av < prev_av)
					av = prev_av - max_av_var_;
				else
					av = prev_av + max_av_var_;
		} 
		
		if(lv > space_->getMaxLinVel())
			lv = space_->getMaxLinVel();
		else if(lv < space_->getMinLinVel())
			lv = space_->getMinLinVel();
		
		if(av > space_->getMaxAngVel())
			av = space_->getMaxAngVel();
		else if(av < (-space_->getMaxAngVel()))
			av = space_->getMaxAngVel()*(-1);
			
		//Dead areas
		/*if(fabs(lv) < 0.08)
			lv = 0.0;
		if(fabs(av) < 0.05)
			av = 0.0;
		*/
		
		//---------------------------------
		State* st = propagateStep(&currentState, lv, av);
		//Check if the state is valid
		if(!space_->isStateValid(st))  {
			motCost = 0.0;
			delete st;
			return false;
		}
		
		Node nextNode(*st);
		nextNode.setCost(space_->getCost(st));
		float mc = motionCost(&currentNode, &nextNode);
		incCost += mc;
		currentState = *st;
		delete st;
		currentNode = nextNode;
		
		numSteps++;
		
		dist = sqrt((wx-currentState.getX())*(wx-currentState.getX()) + (wy-currentState.getY())*(wy-currentState.getY()));
		phi = currentState.getYaw() - wth;
		//Normalize phi
		phi = space_->normalizeAngle(phi, -M_PI, M_PI);
		
		inter_states.push_back(currentState);
		Action a(lv, 0.0, av, 1);
		actions.push_back(a);
		
		prev_lv = lv;
		prev_av = av;
		
	}
	
	if(numSteps == 0){
		return false;
	}
	
	acts = actions;
	motCost = incCost;
	istates = inter_states;
	
	return true;
	
}


//Propagate one step
upo_RRT::State* upo_RRT::Steering::propagateStep(State* st, float lv, float av)
{
	float lin_dist = lv * timeStep_;
	float ang_dist = av * timeStep_;
	float th = st->getYaw() + ang_dist;
	//Normalize th
	th = space_->normalizeAngle(th, -M_PI, M_PI);
	float x = st->getX() + lin_dist*cos(th);
	float y = st->getY() + lin_dist*sin(th); 
	State* result = new State(x, y, th, lv, av);
	return result;
}






