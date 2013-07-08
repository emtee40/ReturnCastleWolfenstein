#ifndef RTCW_OGL_MATRIX_STACK_H
#define RTCW_OGL_MATRIX_STACK_H


#include <stack>

#include <glm/glm.hpp>


namespace rtcw {


class OglMatrixStack {
public:
    typedef glm::mat4 Matrix;


    OglMatrixStack ();
    OglMatrixStack (const OglMatrixStack& that);
    ~OglMatrixStack ();
    OglMatrixStack& operator = (const OglMatrixStack& that);

    // Pops a top matrix from the stack and replaces by it a current one.
    void pop ();

    // Pops a top matrix from the stack, replaces by it a current one and
    // returns it.
    Matrix& pop_and_get ();

    // Pops a top matrix from the stack, replaces by it a current one and
    // returns it's elements.
    float* pop_and_get_items ();

    // Pushes a current matrix into the stack.
    void push ();

    // Pushes a current matrix into the stack and returns it.
    Matrix& push_and_get ();

    // Pushes a current matrix into the stack and replaces the current matrix
    // by a specified one.
    void push_and_set (const Matrix& value);

    // Pushes a current matrix into the stack and replaces the current matrix
    // by a specified one.
    // Note: Items must be specified int column-major order.
    void push_and_set (const float items[16]);

    // Pushes a current matrix into the stack and replaces the current matrix
    // with the identity one.
    void push_and_set_identity ();

    // Returns a current matrix.
    Matrix& get_current ();

    // Returns a current matrix as array.
    float* get_current_items ();

    // Returns elements of the current matrix.
    const float* get_current_items () const;

    // Returns elements of the current matrix.
    const Matrix& get_current () const;

    // Sets a current matrix.
    void set_current (const Matrix& value);

    // Sets a current matrix.
    // Note: Items must be specified int column-major order.
    void set_current (const float items[16]);

    static int get_max_depth ();


private:
    typedef std::stack<Matrix> Stack;

    Stack stack_;
    Matrix current_;
}; // class OglMatrixStack


} // namespace rtcw


#endif // RTCW_OGL_MATRIX_STACK_H