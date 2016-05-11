fips_begin_lib(PhysicsCommon)
    fips_files(
        CollideShapeSetup.h
        RigidBodySetup.h
        SoftBodySetup.h
        Physics.cc Physics.h
        collideShape.cc collideShape.h
        rigidBody.cc rigidBody.h
        softBody.cc softBody.h
        CameraHelper.cc CameraHelper.h
        ShapeRenderer.cc ShapeRenderer.h
    )
    fips_deps(Gfx Input Assets Bullet)
fips_end_lib()
