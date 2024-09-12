# AdobeAfterEffectsPlugins

Some effect implementations since I use them for my own projects. :) For support create an issue and tag me.


NOTE: 32-BIT IMPLEMENTATION ONLY.



Comparisons:

---• Original:
![base](https://github.com/user-attachments/assets/7465d77e-a2e5-4e91-9437-cca0fd46f6a8)![Phos](https://github.com/user-attachments/assets/8a14776a-9ea9-4731-a641-2fa8c37ff299)

---• Anisotropic Kuwahara Filter:
![Anisotropic Kuwahara](https://github.com/user-attachments/assets/e9a7b5bb-4ee2-44e8-a055-4d0d8f207614)![Phos_Kuwahara](https://github.com/user-attachments/assets/edd1a39f-78be-4166-a389-269458dbc3f2)

Adapted from https://github.com/GarrettGunnell/Post-Processing/blob/main/Assets/Kuwahara%20Filter/AnisotropicKuwahara.shader.
(32_bit_only, cpu_only, smart_render = yes, multy_frame_render = yes)

---• Pixel Sorting:
![PixelSorter](https://github.com/user-attachments/assets/c0e448bb-6153-421e-8ce5-f4ee4b2c8be3)![phos_linear](https://github.com/user-attachments/assets/c8967af3-8099-4baf-9a34-3c95f28e365b)

(32_bit_only, cpu_only, smart_render = yes, multy_frame_render = yes)
NOTE: INEFFICIENT SORTING ALGORITHM - BE CAREFULL WHEN SORTING VERY LARGE AREAS

---• Ascii Filter:
![ascii](https://github.com/user-attachments/assets/0475b940-6cf9-481c-a33f-823ba59ff868)![Phos_Ascii](https://github.com/user-attachments/assets/b977dcf8-e68e-432d-888a-26d3770abd8d)

Partly adapted from https://github.com/GarrettGunnell/Post-Processing/tree/main/Assets/ASCII
(32_bit_only, cpu_only, smart_render = yes, multy_frame_render = yes)

---• Dither:
![dither](https://github.com/user-attachments/assets/3d8e4d0a-1429-4b31-8e17-e1c5bd1cda82)![Phos_Dither](https://github.com/user-attachments/assets/e56d4822-7a73-4f05-9a4b-f395ff35e1eb)

NOTE: LONG RENDERING TIME WHEN CHOOSING TO ADAPT COLORS SINCE I JUST SORT A LIST WHICH IS STUPID PERFORMANCE WISE
(32_bit_only, cpu_only, smart_render = yes, multy_frame_render = yes)
