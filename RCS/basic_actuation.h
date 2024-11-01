/*********
  Code to control a car with differential steering using two DC motors.
*********/

// Function to move the car forward
void move_forward() {
  digitalWrite(A1_PIN, LOW);
  digitalWrite(A2_PIN, HIGH);
  digitalWrite(B1_PIN, LOW);
  digitalWrite(B2_PIN, HIGH);
}

// Function to move the car backward
void move_backward() {
  digitalWrite(A1_PIN, HIGH);
  digitalWrite(A2_PIN, LOW);
  digitalWrite(B1_PIN, HIGH);
  digitalWrite(B2_PIN, LOW);
}

// Function to turn left
void turn_left() {
  digitalWrite(A1_PIN, HIGH);
  digitalWrite(A2_PIN, LOW); // Left motor backward
  digitalWrite(B1_PIN, LOW);
  digitalWrite(B2_PIN, HIGH); // Right motor forward
}

// Function to turn right
void turn_right() {
  digitalWrite(A1_PIN, LOW);
  digitalWrite(A2_PIN, HIGH); // Left motor forward
  digitalWrite(B1_PIN, HIGH);
  digitalWrite(B2_PIN, LOW); // Right motor backward
}

// Function to stop the car
void stop_motors() {
  digitalWrite(A1_PIN, LOW);
  digitalWrite(A2_PIN, LOW);
  digitalWrite(B1_PIN, LOW);
  digitalWrite(B2_PIN, LOW);
}

//Function that can be used to handle car actuation based on control input
void actuate(double x, double y) {

  double abs_x = abs(x);
  double abs_y = abs(y);

  if( abs_x < 0.5 && abs_y < 0.5 ) {

    stop_motors();
    Serial.println("Stopping car!");
    return;
  }

  if( y < 0 && abs_x < 0.5 ) {

    move_forward();
    Serial.println("Moving forward!");
    return;
  }

  if( y > 0 && abs_x < 0.5 ) {

    move_backward();
    Serial.println("Moving backward!");
    return;
  }

  if( x < 0 ) {

    turn_left();
    Serial.println("Turning left!");
    return;
  }

  if( x > 0 ) {

    turn_right();
    Serial.println("Turning right!");
    return;
  }
} 