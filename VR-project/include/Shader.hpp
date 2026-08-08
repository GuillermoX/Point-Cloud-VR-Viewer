/*
 * Based in implementation of LearnOpenGL by Joey de Vries
 * Original font: https://learnopengl.com/
 *
 * Modified by: Guillermo Pinteño
 */

#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h> // include glad to get all the required OpenGL headers
  
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
  

class Shader
{
public:
    unsigned int ID;    // The program ID
  
    /**
     * @brief Shader constructor that reads and builds the shader/program
     */
    Shader(const char* vertexPath, const char* fragmentPath);

    /**
     * @brief Use/activate the shader/program in OpenGL context
     */
    void use();
};
  
#endif