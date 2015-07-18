
difference(){
linear_extrude(height=1.6)
circle(38.5, $fn = 200);

linear_extrude(height=1.6)
    translate([8,-28])
square([10.4,17.6]);


linear_extrude(height=1.6)
    translate([-18.4,-28])
square([10.4,17.6]);

}
linear_extrude(height=2.2)
    translate([-17.4,-8])
text("1");

linear_extrude(height=2.2)
    translate([9.3,-8])
text("2");




