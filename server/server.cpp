/*
Name: JULIA DANTAS
SID:  1626063
CCID: jdantas
CMPUT 275, Winter 2022

Assignment part 2: Client/Server Application
*/
#include <iostream>
#include <cassert>
#include <fstream>
#include <sstream>
#include <string>
#include <list>
#include <string.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <cstring>

#include "wdigraph.h"
#include "dijkstra.h"
#define _MSG_MAX_LENGTH 20

struct Point {
  /*
    Point is a structure that hold the longitude and latitude of a given vertex.
    */
  long long lat, lon;
};

// returns the manhattan distance between two points
long long manhattan(const Point& pt1, const Point& pt2) {
  /*
    Computes the Manhattan distance between the two given points
    PARAMETERS:
    pt1: the point from the first vertex
    pt2: the point from the second vertex
    Returns:
    The Manhattan distance between the two given points
    */
  long long dLat = pt1.lat - pt2.lat, dLon = pt1.lon - pt2.lon;
  return abs(dLat) + abs(dLon);
}

// finds the id of the point that is closest to the given point "pt"
int findClosest(const Point& pt, const unordered_map<int, Point>& points) {
  /*
    findClosest find the closest point to the coordinate that was passed in.
    Parameters:
      - pt: the point that we want to find the closest one to
      - points: all the points on our graph
    Return:
      - best.first: the closest point to the one passed

  */
  pair<int, Point> best = *points.begin();

  for (const auto& check : points) {
    // if its distance to the start point is smaller than the current point with the smallest
    // distance to the start coordinate, that point becomes the new "best point"
    if (manhattan(pt, check.second) < manhattan(pt, best.second)) {
      best = check;
    }
  }
  return best.first;
}

// read the graph from the file that has the same format as the "Edmonton graph" file
void readGraph(const string& filename, WDigraph& g, unordered_map<int, Point>& points) {
  /*
    Read Edmonton map data from the provided file and
    load it into the WDigraph object passed to this function.
    Store vertex coordinates in Point struct and map each
    vertex identifier to its corresponding Point struct variable.
    PARAMETERS:
    filename: the name of the file that describes a road network
    graph: an instance of the weighted directed graph (WDigraph) class
    points: a mapping between vertex identifiers and their coordinates
    */
  ifstream fin(filename);
  string line;

  while (getline(fin, line)) {
    // split the string around the commas, there will be 4 substrings either way
    string p[4];
    int at = 0;
    for (auto c : line) {
      if (c == ',') {
        // start new string
        ++at;
      }
      else {
        // append character to the string we are building
        p[at] += c;
      }
    }

    if (at != 3) {
      // empty line
      break;
    }

    if (p[0] == "V") {
      // new Point
      int id = stoi(p[1]);
      assert(id == stoll(p[1])); // sanity check: asserts if some id is not 32-bit
      points[id].lat = static_cast<long long>(stod(p[2])*100000);
      points[id].lon = static_cast<long long>(stod(p[3])*100000);
      g.addVertex(id);
    }
    else {
      // new directed edge
      int u = stoi(p[1]), v = stoi(p[2]);
      g.addEdge(u, v, manhattan(points[u], points[v]));
    }
  }
}

int create_and_open_fifo(const char * pname, int mode) {
  // creating a fifo special file in the current working directory
  // with read-write permissions for communication with the plotter
  // both proecsses must open the fifo before they can perform
  // read and write operations on it
  if (mkfifo(pname, 0666) == -1) {
    cout << "Unable to make a fifo. Ensure that this pipe does not exist already!" << endl;
    exit(-1);
  }

  // opening the fifo for read-only or write-only access
  // a file descriptor that refers to the open file description is
  // returned
  int fd = open(pname, mode);

  if (fd == -1) {
    cout << "Error: failed on opening named pipe." << endl;
    exit(-1);
  }

  return fd;
}

struct header {
  unsigned int len;     // Ignoring byte ordering for now
};

// keep in mind that in part 1, the program should only handle 1 request
// in part 2, you need to listen for a new request the moment you are done
// handling one request
int main() {
  /*
    Builds a graph of the graph specified (here it is the graph of the roads of edmonton) 
    and uses pipes to recieve input from the plotter device and calculates the shortest path from one
    point to another. It then sends the coordinated one byte at a time to the pipe. 
    It writes the path of latitudes and longitudes to the pipe.
    PARAMETERS:
    None
    Returns:
    0 - to signal we are done
    */
  WDigraph graph;
  unordered_map<int, Point> points;

  const char *inpipe = "inpipe";
  const char *outpipe = "outpipe";

  // Open the two pipes
  int in = create_and_open_fifo(inpipe, O_RDONLY);
  cout << "inpipe opened..." << endl;
  int out = create_and_open_fifo(outpipe, O_WRONLY);
  cout << "outpipe opened..." << endl;  

  // build the graph
  readGraph("server/edmonton-roads-2.0.1.txt", graph, points);

  char c;
  Point sPoint, ePoint;

  struct header h = { 0 };

  string p[4];

  char current;
  // the current index to store the latitudes and longitudes of the start and end points
  int idx;
  int number_of_breaks = 0;

  // will be used if 'Q' is passed in by the client to stop accepting input and end the program
  bool please_break = false;

  while(true){
    idx = 0;
    number_of_breaks = 0;
    string p[4];

    while(true){
      read(in, &current, 1);
      if (current == 'Q') {
          // we will exit from the loop because the client wants to stop finding paths and has closed the plotter device
          please_break = true;
          break;
      }
      else if( current == '\n' ){
        number_of_breaks++;
        if (number_of_breaks == 2) {
          break;
        }
        // we are reading in the next coordinate now
        idx++;
      }

      else if (current == ' ') {
        // reading in next longitude
        idx++;
      }

      else {
        p[idx] += current;
      }
      
    }

    if (please_break) {
      // exits from the prgram and closes the pipes
      break;
    }

    sPoint.lat = static_cast<long long>(stod(p[0])*100000);
    sPoint.lon = static_cast<long long>(stod(p[1])*100000);

    ePoint.lat = static_cast<long long>(stod(p[2])*100000);
    ePoint.lon = static_cast<long long>(stod(p[3])*100000);

    int start = findClosest(sPoint, points), end = findClosest(ePoint, points);

    // run dijkstra's algorithm, this is the unoptimized version that
    // does not stop when the end is reached but it is still fast enough
    unordered_map<int, PIL> tree;
    dijkstra(graph, start, tree);

    // NOTE: in Part II you will use a different communication protocol than Part I
    // So edit the code below to implement this protocol

    // no path
    if (tree.find(end) == tree.end()) {
      // there are no paths if we enter here. but we are not required to write
      // anything to the pipe
    }

    else {
      // read off the path by stepping back through the search tree
      list<int> path;
      while (end != start) {
        path.push_front(end);
        end = tree[end].first;
      }
      path.push_front(start);


      for (int v : path) {
        char message[_MSG_MAX_LENGTH];
        memset(message, 0, _MSG_MAX_LENGTH);

        // formats the coordinates to be in the correct form
        string latitude = to_string(points[v].lat);
        latitude.insert(2, 1, '.');
        string longitude = to_string(points[v].lon);
        longitude.insert(4, 1, '.');

        string current_message = latitude + " " + longitude + "\n";

        int i = 0;
        char current_char;

        while(1){
          // we are writing one byte at a time
          current_char = current_message[i];
          if( current_char == '\n' ){
            write(out, &current_char, 1);
            break;
          }
          write(out, &current_char, 1); 
          i++;
        }

      }
    }

    char end_message[2];
    // signals we are done writing the path
    end_message[0] = 'E';
    end_message[1] = '\n';

    int i = 0;
    char current_char;

    while(1){
      current_char = end_message[i];
      if( current_char == '\n' ){
        write(out, &current_char, 1);
        break;
      }
      write(out, &current_char, 1); 
      i++;
    }
  }
  
  // close pipe from the write end 
  close(out);
  close(in);
  // reclaim the backing file 
  unlink(inpipe);
  unlink(outpipe);

  return 0;
}
