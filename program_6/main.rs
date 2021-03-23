use std::env; // to get arugments passed to the program
use std::thread; //for threads

/*
* Print the number of partitions and the size of each partition
* @param vs A vector of vectors
*/
fn print_partition_info(vs: &Vec<Vec<usize>>){
    println!("Number of partitions = {}", vs.len());
    for i in 0..vs.len(){
        println!("\tsize of partition {} = {}", i, vs[i].len());
    }
}

/*
* Create a vector with integers from 0 to num_elements -1
* @param num_elements How many integers to generate
* @return A vector with integers from 0 to (num_elements - 1)
*/
fn generate_data(num_elements: usize) -> Vec<usize>{
    let mut v : Vec<usize> = Vec::new();    //create a new vector
    for i in 0..num_elements {
        v.push(i);
    }
    return v;
}

/*
* Partition the data in the vector v into 2 vectors
* @param v Vector of integers
* @return A vector that contains 2 vectors of integers

*/
fn partition_data_in_two(v: &Vec<usize>) -> Vec<Vec<usize>>{
    let partition_size = v.len() / 2;   //get the middle
    // Create a vector that will contain vectors of integers
    let mut xs: Vec<Vec<usize>> = Vec::new();

    // Create the first vector of integers
    let mut x1 : Vec<usize> = Vec::new();
    // Add the first half of the integers in the input vector to x1
    for i in 0..partition_size{
        x1.push(v[i]);
    }
    // Add x1 to the vector that will be returned by this function
    xs.push(x1);

    // Create the second vector of integers
    let mut x2 : Vec<usize> = Vec::new();
    // Add the second half of the integers in the input vector to x2
    for i in partition_size..v.len(){
        x2.push(v[i]);
    }
    // Add x2 to the vector that will be returned by this function
    xs.push(x2);
    // Return the result vector
    xs
}

/*
* Sum up the all the integers in the given vector
* @param v Vector of integers
* @return Sum of integers in v
* Note: this function has the same code as the reduce_data function.
*       But don't change the code of map_data or reduce_data.
*/
fn map_data(v: &Vec<usize>) -> usize{
    let mut sum = 0;
    for i in v{
        sum += i;
    }
    sum
}

/*
* Sum up the all the integers in the given vector
* @param v Vector of integers
* @return Sum of integers in v
*/
fn reduce_data(v: &Vec<usize>) -> usize{
    let mut sum = 0;
    for i in v{
        sum += i;
    }
    sum
}

/*
* A single threaded map-reduce program
*/
fn main() {

    // Use std::env to get arguments passed to the program
    let args: Vec<String> = env::args().collect();
    if args.len() != 3 {
        println!("ERROR: Usage {} num_partitions num_elements", args[0]);
        return;
    }
    let num_partitions : usize = args[1].parse().unwrap();
    let num_elements : usize = args[2].parse().unwrap();
    if num_partitions < 1{
      println!("ERROR: num_partitions must be at least 1");
        return;
    }
    if num_elements < num_partitions{
        println!("ERROR: num_elements cannot be smaller than num_partitions");
        return;
    }

    // Generate data.
    let v = generate_data(num_elements);

    // PARTITION STEP: partition the data into 2 partitions
    let xs = partition_data_in_two(&v);

    // Print info about the partitions
    print_partition_info(&xs);

    let mut intermediate_sums : Vec<usize> = Vec::new();

    // MAP STEP: Process each partition

    // CHANGE CODE START: Don't change any code above this line

    // create two threads. We use move here to transfer ownership of the xs vector. To avoid error and allow for thread 2 to use xs after it is moved to thread 1, use clones of xs[0] & [1] for the threads

    //create the threads that call map_data, this code was adapted from the OS1 rust modules
    let _c1 = xs[0].clone();
    let _c2 = xs[1].clone();

    let thread_1 = thread::spawn(move || map_data(&_c1));
    let thread_2 = thread::spawn(move || map_data(&_c2));
    //get join values
    let _r1 = thread_1.join().unwrap();
    let _r2 = thread_2.join().unwrap();


    //push results of threads running map_data to intermediate_sums
    intermediate_sums.push(_r1);
    intermediate_sums.push(_r2);

    // CHANGE CODE END: Don't change any code below this line until the next CHANGE CODE comment

    // Print the vector with the intermediate sums
    println!("Intermediate sums = {:?}", intermediate_sums);

    // REDUCE STEP: Process the intermediate result to produce the final result
    let sum = reduce_data(&intermediate_sums);
    println!("Sum = {}", sum);

    // CHANGE CODE: Add code that does the following:
    // 1. Calls partition_data to partition the data into equal partitions
    // 2. Calls print_partition_info to print info on the partitions that have been created
    // 3. Creates one thread per partition and uses each thread to concurrently process one partition
    // 4. Collects the intermediate sums from all the threads
    // 5. Prints information about the intermediate sums
    // 5. Calls reduce_data to process the intermediate sums
    // 6. Prints the final sum computed by reduce_data

    //call partition_data and print_partition_info
    let _xs = partition_data(num_partitions, &v);
    print_partition_info(&_xs);

    let mut inter_sums : Vec<usize> = Vec::new();
    //to concurrently create one thread per partition, we need to create them all then get their join vals, so we
    //need a vector of threads to store them all
    //this is from the hints section
    let mut thread_vec : Vec<std::thread::JoinHandle<usize>> = Vec::new();
    //loop through the num of partitions, using a similar process to what was done in part one of this assignment
    for i in 0..num_partitions{
        //clone current vector of _xs 
        let _xs_c = _xs[i].clone();
        //spawn thread and push it to thread vector
        thread_vec.push(thread::spawn(move || map_data(&_xs_c)));
    }
    //loop through partitions again, capturing the join values of each thread. Items are poped from vectors with 'remove'
    for i in 0..num_partitions{
        let join_val = thread_vec.remove(0).join().unwrap();
        //push this value to intermediate sums
        inter_sums.push(join_val);
    }

    //printing intermediate and final sums
    println!("Intermediate sums = {:?}", inter_sums);
    let _sum = reduce_data(&inter_sums);
    println!("Sum = {}", _sum);

}

/*
* CHANGE CODE: code this function
* Note: Don't change the signature of this function
*
* Partitions the data into a number of partitions such that
* - the returned partitions contain all elements that are in the input vector
* - if num_elements is a multiple of num_partitions, then all partitions must have equal number of elements
* - if num_elements is not a multiple of num_partitions, some partitions can have one more element than other partitions
*
* @param num_partitions The number of partitions to create
* @param v The data to be partitioned
* @return A vector that contains vectors of integers
* 
*/
fn partition_data(num_partitions: usize, v: &Vec<usize>) -> Vec<Vec<usize>>{
    //similar to, and based off, the partition_data_in_two function
    //first, create a vector that will hold vectors of ints
    let mut _xs: Vec<Vec<usize>> = Vec::new();
    //get len of v
    let length = v.len();
    //check if num_elems is a multiple of num_partitions
    let mut check = length % num_partitions;
    //get size of partitions
    let part_size = length / num_partitions;
    //variable to store index of the passed in vector
    let mut index = 0;
    //loop through the number of partitions, for each iteration, create a vector of ints, and push to it the current index of the passed in vector part_size times,
    //or part_size + 1 times, if num_elems is not a multiple of num_partitions
    for i in 0..num_partitions{
        //create the current vector
        let mut curr_vec: Vec<usize> = Vec::new();
        //check if all partitions will have equal num of elements or not
        if check > 0{
            //not a multiple, so this vector will have 1 elem more than some others
            for j in 0..part_size + 1{
                //push to current vector the current index of v
                curr_vec.push(v[index]);
                index += 1;
            }
            check -= 1;
        }
        else{
            for j in 0..part_size{
                curr_vec.push(v[index]);
                index += 1;
            }
        }
        //now that curr_vec has been filled with the appropriate number of elems, push it to the vector of vectors that this function returns
        _xs.push(curr_vec);
    }
    //return the vector of vectors
    _xs
    
}
