from os import scandir
import os
import re

# Get files and sort numerically
def get_files_from_folder(folder_path):
    # Extract numerical values for sorting
    def extract_numbers(filename):
        numbers = re.findall(r'\d+', filename)  # Extract all numbers
        return list(map(int, numbers))  # Convert to integers for sorting
    
    return sorted(
        [entry.path.replace("\\", "/") for entry in os.scandir(folder_path) if entry.is_file()],  # Normalize path
        key=lambda x: extract_numbers(os.path.basename(x))  # Extract numbers from filename for sorting
    )
    
# find distance from the start vertex to all other vertices in the graph
def find_distance(graph, start_vertex):
    visited = {start_vertex: 0}
    queue = [start_vertex]
    while queue:
        current_vertex = queue.pop(0)
        current_distance = visited[current_vertex]
        for neighbor in graph[current_vertex]:
            if neighbor not in visited:
                visited[neighbor] = current_distance + 1
                queue.append(neighbor)
    return visited

# find the distance from all vertex to the first vertex in the graph
def find_distance_to_first(graph, start_vertex):
    def reverse_graph(graph):
        reversed_graph = {i: [] for i in graph}
        for v in graph:
            for u in graph[v]:
                reversed_graph[u].append(v)
        return reversed_graph
    
    distance = find_distance(reverse_graph(graph), start_vertex)
    return distance


# For sat solver interrupt function
def interrupt(s): s.interrupt()
