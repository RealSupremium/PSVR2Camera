# PSVR2 Camera Viewer

This is a simple Windows app for viewing the raw camera feeds from the PSVR2 headset. It uses shared memory to access the image data provided by the PSVR2 driver via `ShareManager` and renders it using DirectX 11. You must have a PSVR2 plugged in and SteamVR running.

## Features

-   **Live Camera Feed:** Displays the feeds of the bottom cameras on the PSVR2 at 60FPS. The top ones are not sent to PC.
-   **Lens Undistortion:** Applies lens distortion correction to the camera images. Can be toggled on and off.
-   **Multiple View Modes:** You can view the left camera, right camera, or both side-by-side.

## Controls

-   **`1`**: Switch to the left camera view.
-   **`2`**: Switch to the right camera view.
-   **`3`**: Switch to the side-by-side view.
-   **`+`**: Zoom in.
-   **`-`**: Zoom out.
-   **`D`**: Toggle distortion.
