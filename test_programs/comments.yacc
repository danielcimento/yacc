// First, we set our variable "a" to 3
a = 3;
// Then, we increment if "a" is greater than 2
if (a > 2) {   // This should always be true
    a++; 
} else { 
    a--; 
} 

// We can even do embedded comments, which are super useful for placeholders, etc
if (a > 3 /*|| true*/) {
    a++;    // Comments can come at the end of lines, too!
} 

a;

// All this will */ be ignored!
/* aren't // compilers great? */