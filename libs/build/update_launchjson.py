#!/usr/bin/env python3

import json
import os
import sys

def update_launch_json(launch_config_path, target_name, executable_path, arguments=None):
    """
    Updates the VS Code launch.json file with an lldb launch configuration.
    Checks if the target exists and updates it or creates a new configuration if it doesn't.

    Args:
        launch_config_path (str): Path to the launch.json file.
        target_name (str): Name of the target/launch configuration.
        executable_path (str): Path to the executable to launch.
        arguments (list, optional): List of arguments to pass to the executable. Defaults to None.
    """

    # Ensure the directory exists
    os.makedirs(os.path.dirname(launch_config_path), exist_ok=True)

    # Create a default launch.json if it doesn't exist 
    if not os.path.exists(launch_config_path):
        with open(launch_config_path, 'w') as f:
            json.dump({'version': '0.2.0', 'configurations': []}, f)
      
    # Load the existing launch.json
    try:
        with open(launch_config_path, 'r') as f:
            launch_config = json.load(f)

        configurations = launch_config.get('configurations', [])

        target_exists = False
        for config in configurations:
            if config.get('name') == target_name:
                # Update target config
                config['program'] = executable_path
                if arguments:
                    config['args'] = arguments
                target_exists = True
                break

        if not target_exists:
            new_config = {
                'name': target_name,
                'type': 'lldb',
                'request': 'launch',
                'program': executable_path,
                'args': arguments if arguments else [],
                'cwd': '${workspaceFolder}',
            }
            configurations.append(new_config)

        # Update configuration
        launch_config['configurations'] = configurations

        with open(launch_config_path, 'w') as f:
            json.dump(launch_config, f, indent=4)

        print(f"Launch configuration '{target_name}' updated/created in '{launch_config_path}'.")

    except FileNotFoundError:
        # If launch.json doesn't exist, create an empty one
        with open(launch_config_path, 'w') as f:
            json.dump({'configurations': []}, f, indent=4)
        print(f"launch.json created at '{launch_config_path}'.")
    except Exception as e:
        print(f"An error occurred: {e}")


if __name__ == '__main__':
    # get meson source root evironment variable
    meson_source_root = os.environ.get('MESON_SOURCE_ROOT')
    launch_json_path = os.path.join(meson_source_root, '.vscode', 'launch.json')

    # if len(sys.argv) != 3:
    #     raise Exception('Target name and executable path are required')

    target_name = sys.argv[1]
    executable_path = sys.argv[2]
    arguments = []
    
    update_launch_json(launch_json_path, target_name, executable_path, arguments)
