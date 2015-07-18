

difference(){
// the top surface
linear_extrude(height=2.4)
circle(39.5, $fn = 200);
//the ring for the gumball top
rotate_extrude()
    translate([12.5,0,0])
    square(1.5);
linear_extrude(height=2.4)    
    circle(3);
}
// the walls
difference(){
    linear_extrude(height=35)
circle(39.5, $fn = 200);
    linear_extrude(height=35)
circle(38.5, $fn = 100);
}

// the step
difference(){
    linear_extrude(height=20)
circle(39);
    linear_extrude(height=20)
circle(37);
}


