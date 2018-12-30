#include "Globals.h"
#include "Application.h"
#include "ModulePlayer.h"
#include "Primitive.h"
#include "ModuleInput.h"
#include "PhysVehicle3D.h"
#include "PhysBody3D.h"

ModulePlayer::ModulePlayer(Application* app, bool start_enabled) : Module(app, start_enabled), vehicle(NULL)
{
	turn = acceleration = brake = 0.0F;
}

ModulePlayer::~ModulePlayer()
{}

// Load assets
bool ModulePlayer::Start()
{
	LOG("Loading player");

	VehicleInfo car;
	
	// Car properties ----------------------------------------
	car.chassis_size.Set(2.5F, 0.5F, 4);
	car.chassis_offset.Set(0, 1, 0);
	car.mass = 500.0F;
	car.suspensionStiffness = 15.88F;
	car.suspensionCompression = 0.83F;
	car.suspensionDamping = 0.88F;
	car.maxSuspensionTravelCm = 1000.0F;
	car.frictionSlip = 50.5F;
	car.maxSuspensionForce = 6000.0F;

	// Wheel properties ---------------------------------------
	float connection_height = 1.2F;
	float wheel_radius = 0.6F;
	float wheel_width = 0.7F;
	float suspensionRestLength = 1.2F;

	// Don't change anything below this line ------------------
	float half_width = car.chassis_size.x*0.5F;
	float half_length = car.chassis_size.z*0.5F;

	direction = {0, -1, 0};
	axis = { -1, 0, 0 };
	
	car.num_wheels = 4;
	car.wheels = new Wheel[4];

	// FRONT-LEFT ------------------------
	car.wheels[0].connection.Set(half_width - 0.3F * wheel_width, connection_height, half_length - wheel_radius);
	car.wheels[0].direction = direction;
	car.wheels[0].axis = axis;
	car.wheels[0].suspensionRestLength = suspensionRestLength;
	car.wheels[0].radius = wheel_radius;
	car.wheels[0].width = wheel_width;
	car.wheels[0].front = true;
	car.wheels[0].drive = true;
	car.wheels[0].brake = false;
	car.wheels[0].steering = true;

	// FRONT-RIGHT ------------------------
	car.wheels[1].connection.Set(-half_width + 0.3F * wheel_width, connection_height, half_length - wheel_radius);
	car.wheels[1].direction = direction;
	car.wheels[1].axis = axis;
	car.wheels[1].suspensionRestLength = suspensionRestLength;
	car.wheels[1].radius = wheel_radius;
	car.wheels[1].width = wheel_width;
	car.wheels[1].front = true;
	car.wheels[1].drive = true;
	car.wheels[1].brake = false;
	car.wheels[1].steering = true;

	// REAR-LEFT ------------------------
	car.wheels[2].connection.Set(half_width - 0.3F * wheel_width, connection_height, -half_length + wheel_radius);
	car.wheels[2].direction = direction;
	car.wheels[2].axis = axis;
	car.wheels[2].suspensionRestLength = suspensionRestLength;
	car.wheels[2].radius = wheel_radius;
	car.wheels[2].width = wheel_width;
	car.wheels[2].front = false;
	car.wheels[2].drive = false;
	car.wheels[2].brake = true;
	car.wheels[2].steering = false;

	// REAR-RIGHT ------------------------
	car.wheels[3].connection.Set(-half_width + 0.3F * wheel_width, connection_height, -half_length + wheel_radius);
	car.wheels[3].direction = direction;
	car.wheels[3].axis = axis;
	car.wheels[3].suspensionRestLength = suspensionRestLength;
	car.wheels[3].radius = wheel_radius;
	car.wheels[3].width = wheel_width;
	car.wheels[3].front = false;
	car.wheels[3].drive = false;
	car.wheels[3].brake = true;
	car.wheels[3].steering = false;

	vehicle = App->physics->AddVehicle(car);
	vehicle->SetState(PhysBody3D::Tag::PLAYER);
	vehicle->SetPos(0, 1, 0);
	vehicle->collision_listeners.add(App->scene_intro);

	App->camera->Position.Set(vehicle->GetPosition().x - vehicle->GetForwardVector().x*CAMERA_OFFSET_X,
		vehicle->GetPosition().y + CAMERA_OFFSET_Y,
		vehicle->GetPosition().z - vehicle->GetForwardVector().z * CAMERA_OFFSET_Z);
	App->camera->LookAt(vehicle->GetPosition());

	car.num_wheels = 0;
	//car.mass = 1.0f;
	ghost = App->physics->AddVehicle(car);
	ghost->SetState(PhysBody3D::Tag::GHOST);
	ghost->SetPos(15, 1, 0);
	ghost->SetAsSensor(true);
	ghost->vehicle->getRigidBody()->setGravity(btVector3(0, 0, 0));
	//ghost->vehicle->getRigidBody()->setFlags(ghost->vehicle->getRigidBody()->getFlags() | btRigidBodyFlags::BT_DISABLE_WORLD_GRAVITY | btRigidBody::CF_STATIC_OBJECT);

	//SFx
	fx_horn = App->audio->LoadFx("Audio/SFX/carhorn.wav");
	fx_crash = App->audio->LoadFx("Audio/SFX/crash.wav");
	fx_racing = App->audio->LoadFx("Audio/SFX/racing.wav");
	fx_start_car = App->audio->LoadFx("Audio/SFX/start_car.wav");
	fx_nitro_pick_up = App->audio->LoadFx("Audio/SFX/nitro_pick_up.wav");
	fx_nitro = App->audio->LoadFx("Audio/SFX/nitro.wav");
	fx_car_engine = App->audio->LoadFx("Audio/SFX/car_engine.wav");
	fx_checkpoint = App->audio->LoadFx("Audio/SFX/checkpoint.wav");
	App->audio->PlayMusic("Audio/Music/Ambientation.ogg");


	return true;
}

// Update: draw background
update_status ModulePlayer::Update(float dt)
{
	current_time = SDL_GetTicks() - start_time;

	if (turn_on_car)
	{
		App->audio->PlayFx(fx_start_car);
		turn_on_car = false;
	}

	// Inputs
	if (App->input->GetKey(SDL_SCANCODE_LALT) == KEY_DOWN) {
		App->audio->PlayFx(fx_horn);
		LOG("%.2f %.2f %.2f", vehicle->GetPosition().x, vehicle->GetPosition().y, vehicle->GetPosition().z);
	}
	if (App->input->GetKey(SDL_SCANCODE_F2) == KEY_DOWN) {
		switch (checkpoint_value)
		{
		case 0:
			vehicle->SetPos(0, 1, 0);
			break;
		case 1:
			vehicle->SetPos(-129.81F, 1, 128.58F);
			break;
		case 2:
			vehicle->SetPos(-29.24F, 1, -284.81F);
			break;
		case 3:
			vehicle->SetPos(60.71F, 1, -200.62F);
			break;
		case 4:
			vehicle->SetPos(-129.81F, 1, 128.58F);
			break;
		default:
			vehicle->SetPos(0, 0.44F, -26.71F);
			break;
		}
	}
	if (App->input->GetKey(SDL_SCANCODE_F4) == KEY_DOWN) {
		SaveGhostData(!save_ghost_data);
	}
	if (App->input->GetKey(SDL_SCANCODE_F5) == KEY_DOWN) {
		//ghost->vehicle->getRigidBody()->setCenterOfMassTransform
		ghost->vehicle->getRigidBody()->setLinearVelocity(btVector3(0, 0, 0));
		ghost->vehicle->getRigidBody()->setWorldTransform(vehicle->vehicle->getRigidBody()->getWorldTransform());
		ghost->SetPos(ghost->GetPosition() + vec3(0, 10, 0));
	}

	if (save_ghost_data && timer.Read() > 100) {

		ghost_pos.PushBack(vehicle->vehicle->getRigidBody()->getWorldTransform());
		timer.Start();
	}

	if (App->input->GetKey(SDL_SCANCODE_P) == KEY_DOWN) {
		timer.Start();
		save_ghost_data = false;
		path_ghost = true;
		iterator_ghost = 0;
		ghost->vehicle->getRigidBody()->setWorldTransform(ghost_pos[iterator_ghost]);
	}

	if (path_ghost && timer.Read() > 100) {
		if (iterator_ghost + 1 >= ghost_pos.Count()) {
			path_ghost = false;
			ghost_pos.Clear();
			timer.Stop();
		}
		else {
			ghost->vehicle->getRigidBody()->setWorldTransform(ghost_pos[++iterator_ghost]);
			timer.Start();
		}
	}
	//-----------------------------------------------------------

	if (vehicle->GetUpperVector().y == -1) {
		btTransform a;
		a.setIdentity();
		vehicle->vehicle->getRigidBody()->setWorldTransform(a);
	}

	//Car control	---------------------------------------------------------------
	turn = acceleration = brake = 0.0F;

	if (vehicle->GetKmh() >= 1 && accelerating)
		acceleration = -MAX_ACCELERATION;
	else if (vehicle->GetKmh() <= -1 && decelerating)
		acceleration = MAX_ACCELERATION;
	else
	{
		acceleration = 0;
		accelerating = false;
		decelerating = false;

		if (current_time >= 5000)
		{
			App->audio->PlayFx(fx_car_engine);
			start_time = SDL_GetTicks();
		}
	}

	if(App->input->GetKey(SDL_SCANCODE_UP) == KEY_REPEAT)
	{
		if (vehicle->GetKmh() >= 100)
			acceleration = 0;
		else
			acceleration = MAX_ACCELERATION;

		if (current_time >= 5000)
		{
			App->audio->PlayFx(fx_racing, 0, true);
			start_time = SDL_GetTicks();
		}
		accelerating = true;
	}

	if(App->input->GetKey(SDL_SCANCODE_LEFT) == KEY_REPEAT)
	{
		if(turn < TURN_DEGREES)
			turn +=  TURN_DEGREES;
	}

	if(App->input->GetKey(SDL_SCANCODE_RIGHT) == KEY_REPEAT)
	{
		if(turn > -TURN_DEGREES)
			turn -= TURN_DEGREES;
	}

	if(App->input->GetKey(SDL_SCANCODE_DOWN) == KEY_REPEAT)
	{
		if (vehicle->GetKmh() > 0)
			brake = BRAKE_POWER;
		else
		{
			brake = NULL;
			if (vehicle->GetKmh() <= -50)
				acceleration = 0;
			else
				acceleration = MAX_DECCELERATION;

			if (current_time >= 5000)
			{
				App->audio->PlayFx(fx_car_engine);
				start_time = SDL_GetTicks();
			}

			decelerating = true;
		}

		
	}
	
	if (App->input->GetKey(SDL_SCANCODE_SPACE) == KEY_REPEAT)
	{
		NitroSpeed();
	}

	vehicle->ApplyEngineForce(acceleration);
	vehicle->Turn(turn);
	vehicle->Brake(brake);

	vehicle->Render();
	ghost->Render();

	if (!App->scene_intro->camera_free) {
		App->camera->Position.Set(vehicle->GetPosition().x - vehicle->GetForwardVector().x*CAMERA_OFFSET_X,
			vehicle->GetPosition().y + CAMERA_OFFSET_Y,
			vehicle->GetPosition().z - vehicle->GetForwardVector().z * CAMERA_OFFSET_Z);
		App->camera->LookAt(vehicle->GetPosition());
	}

	return UPDATE_CONTINUE;
}

// Unload assets
bool ModulePlayer::CleanUp()
{
	LOG("Unloading player");

	return true;
}

void ModulePlayer::NitroSpeed()
{
	if (nitro)
	{
		App->audio->PlayFx(fx_nitro);
		start_nitro = SDL_GetTicks(); 
		nitro = false;
		vehicle->nitro_off = 255;
		vehicle->nitro_on = 0;
	}


	current_time = SDL_GetTicks() - start_nitro;
	
	if (current_time <= 1500)
	{
		if (vehicle->GetKmh() <= 150)
			acceleration = MAX_ACCELERATION * 2;
		else
			acceleration = 0;
	
		accelerating = true;

	}
	
}

bool ModulePlayer::SaveGhostData(bool to_save)
{
	if (to_save && !save_ghost_data) {
		save_ghost_data = true;
		timer.Start();
		LOG("Starting saving ghost data");
		return true;
	}
	else if (to_save && save_ghost_data) { //already saving data
		LOG("Warning: already saving ghost data");
		return false;
	}
	else if (!to_save && !save_ghost_data) { //don't saving data
		LOG("Warning: already don't saving ghost data");
		return false;
	}
	else if (!to_save && !save_ghost_data) {
		save_ghost_data = false;
		timer.Stop();
		ghost_pos_prev = ghost_pos;
		ghost_pos.Clear();
		LOG("Stopping saving ghost data, time recording %i",timer.Read()/1000);
		return true;
	}

	return false;
}

