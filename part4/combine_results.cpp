#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
using namespace std;
// Function to read the CSV file and calculate the average
double calculate_average(const string& filename) {
    ifstream file(filename);
  
    string line;
    double sum = 0.0;
    int count = 0;

    // Read each line from the file
    while (getline(file, line)) {
        stringstream ss(line);
        string value;
        
        // Parse the completion time value from the line (assuming a single value per line)
        while (getline(ss, value, ',')) {
            sum += stod(value);  // Convert string to double and add to sum
            count++;
        }
    }

    file.close();

    // Return the average
    return (count > 0) ? (sum / count) : 0.0;
}

// Function to save the two averages to a new text file
void save_averages_to_file(const string& output_filename, double avg1, double avg2) {
    ofstream output_file(output_filename);
    
    if (!output_file.is_open()) {
        cerr << "Error: Could not open the output file " << output_filename << endl;
        return;
    }

    // Save the averages as comma-separated values
    output_file << avg1 << "," << avg2 << endl;

    output_file.close();
    cout << "Averages saved to " << output_filename << endl;
}

int main() {
    // File paths for the two CSV files
    string file1 = "client_time_fifo.csv";  // Replace with your first CSV file path
    string file2 = "client_time_rr.csv";  // Replace with your second CSV file path
    string output_file = "averages.txt";  // Output file

    // Calculate averages for the two files
    double avg1 = calculate_average(file1);
    double avg2 = calculate_average(file2);

    // Save the averages to the output file as comma-separated values
    save_averages_to_file(output_file, avg1, avg2);

    return 0;
}
