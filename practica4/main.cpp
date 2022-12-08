#include <iostream>
#include "camera.hpp"

int main (int argc, char* argv[]) {

    Mesh m({
        Triangle({Vector3(-0.5,0,0.5), Vector3(0,0.5,0), Vector3(0,-0.5,0)},   RGB(205, 170, 240)),
        Triangle({Vector3(0,-0.5,0), Vector3(0,0.5,0), Vector3(0.5,0,0.5)},    RGB(205, 170, 240)),
        Triangle({Vector3(-0.5,0,0.5), Vector3(0,0.5,0), Vector3(0.5,0,0.5)},  RGB(205, 170, 240)),
        Triangle({Vector3(-0.5,0,0.5), Vector3(0,-0.5,0), Vector3(0.5,0,0.5)}, RGB(205, 170, 240)),
    });

    Camera C(Vector3(0,0,-3.5), Vector3(-1,0,0), Vector3(0,1,0), Vector3(0,0,3), 512, 512, 16);
    Scene scene({
        // El main escenario
        std::make_shared<Plane>(Plane(1, Vector3(1,0,0) , RGB(200,0,0))),    //Plano de la izquierda
        std::make_shared<Plane>(Plane(1, Vector3(-1,0,0), RGB(0,200,0))),   //Plano de la derecha
        std::make_shared<Plane>(Plane(1, Vector3(0,1,0) , RGB(250,250,250))),    //Plano de abajo
        std::make_shared<Plane>(Plane(1, Vector3(0,-1,0), RGB(250,250,250))),   //Plano de arriba
        std::make_shared<Plane>(Plane(1, Vector3(0,0,-1), RGB(250,250,250))),     //Plano de atrás
        //std::make_shared<Sphere>(Sphere(Vector3(-0.5,0.3,-0.6), 0.3, RGB(255,0,200))),  //Esfera de la izquierda
        std::make_shared<Sphere>(Sphere(Vector3(-0.5,-0.7,0.25), 0.3, RGB(220,145,220))),  //Esfera de la izquierda
        std::make_shared<Sphere>(Sphere(Vector3(0.5,-0.7,-0.25), 0.3, RGB(145,220,220))),  //Esfera de la derecha
        //std::make_shared<Mesh>(Mesh("../src/ply/cow.ply") * Matrix3Scale(0.4) * Matrix3Translation(0, -0.6, 0) * Matrix3Rotation(Y_ROT, 45)),
        //std::make_shared<Mesh>(Mesh("../src/ply/mario.ply") * Matrix3Scale(0.07) * Matrix3Rotation(Y_ROT, -135) * Matrix3Translation(0,-0.1,0)),
        //std::make_shared<Mesh>(m * Matrix3Rotation(X_ROT, -45)),
    }, {
        std::make_shared<Light>(Light(Vector3(0,0.5,0), RGB(1.0f,1.0f,1.0f)))
    });

    C.render(scene, PATH_TRACING);
    C.export_render();

}