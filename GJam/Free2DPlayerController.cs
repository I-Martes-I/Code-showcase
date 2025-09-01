using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class Free2DPlayerController : MonoBehaviour
{
    [Header("Movement Settings")]
    public float moveSpeed = 5f;
    public float jumpForce = 10f;
    
    [Header("Variable Jump Settings")]
    public float maxJumpHoldTime = 0.3f; // Maximum time player can hold jump for bonus
    public float additionalJumpForce = 15f; // Additional upward force applied while holding jump
    public float jumpHoldGravityScale = 0.5f; // Reduced gravity while holding jump
    
    [Header("Ground Check")]
    public Transform groundCheck;
    public float groundCheckRadius = 0.2f;
    public LayerMask groundLayer;
    public LayerMask boxLayer;

    [Header("Head Check")]
    public Transform headCheck;
    public float headCheckRadius = 0.2f;
    public LayerMask obstacleLayer;
    public float counterForce = -10f;
    private bool isHeadHit = false;
    public float yVelocityThreshold = 2f;

    #region VELOCITY_DEBUG - Remove this region when done testing
    [Header("DEBUG - Velocity Tracking")]
    [SerializeField] private float currentVelocityY;
    [SerializeField] private float currentVelocityMagnitude;
    [SerializeField] private bool showVelocityOnScreen = true;
    #endregion

    private Rigidbody2D rb;
    private bool isGrounded;
    private float horizontalInput;
    private bool jumpPressed;
    private bool isHoldingJump;
    private float jumpHoldTime;
    private float originalGravityScale;
    private bool wasGrounded;
    private int facing = 1; // 1 = right, -1 = left
    
    void Start()
    {
        rb = GetComponent<Rigidbody2D>();
        originalGravityScale = rb.gravityScale;
        facing = transform.localScale.x < 0 ? -1 : 1;
    }

    void Update()
    {
        wasGrounded = isGrounded;
        horizontalInput = Input.GetAxis("Horizontal");
        CheckGrounded();
        HandleJumpReset();
        HandleJumpInput();
        
        #region VELOCITY_DEBUG - Remove this region when done testing
        // Update debug velocity values for Inspector
        currentVelocityY = rb.velocity.y;
        currentVelocityMagnitude = rb.velocity.magnitude;
        #endregion
    }
    
    void FixedUpdate()
    {
        // horizontal movement
        rb.velocity = new Vector2(horizontalInput * moveSpeed, rb.velocity.y);
        Jump();
        
    }

    void LateUpdate()
    {
        TurnAround();
    }

    private void TurnAround()
    {
        // Update facing direction based on horizontal input
        if (horizontalInput != 0)
        {
            facing = horizontalInput > 0 ? 1 : -1;
            Vector3 localScale = transform.localScale;
            localScale.x = Mathf.Abs(localScale.x) * facing; // Ensure scale is always positive
            transform.localScale = localScale;
        }
    }

    private void CheckGrounded()
    {
        isGrounded = Physics2D.OverlapCircle(groundCheck.position, groundCheckRadius, groundLayer) || Physics2D.OverlapCircle(groundCheck.position, groundCheckRadius, boxLayer);
    }

    private void HandleJumpReset()
    {
        bool headCollision = Physics2D.OverlapCircle(headCheck.position, headCheckRadius, obstacleLayer);
        if (isGrounded && !wasGrounded)
        {
            isHoldingJump = false;
            isHeadHit = false;
            rb.gravityScale = originalGravityScale;
        }
        else if (headCollision && rb.velocity.y > 0 && !isGrounded)
        {
            isHoldingJump = false;
            isHeadHit = true;
            if (rb.velocity.y > yVelocityThreshold)
            {
                rb.velocity = new Vector2(rb.velocity.x, counterForce); // Stop upward movement
            }
            rb.gravityScale = originalGravityScale;
        }
    }

    private void HandleJumpInput()
    {
        if ((Input.GetKeyDown(KeyCode.W) || Input.GetKeyDown(KeyCode.UpArrow) || Input.GetKeyDown(KeyCode.Space)) && isGrounded)
        {
            jumpPressed = true;
            isHoldingJump = true;
            jumpHoldTime = 0f;
        }
        
        // holding jump button in the air
        if (isHoldingJump && !isGrounded && (Input.GetKey(KeyCode.W) || Input.GetKey(KeyCode.UpArrow) || Input.GetKey(KeyCode.Space)))
        {
            jumpHoldTime += Time.deltaTime;
            
            // Ограничиваем максимальное время удержания
            if (jumpHoldTime > maxJumpHoldTime)
            {
                isHoldingJump = false;
                rb.gravityScale = originalGravityScale;
            }
        }
        
        // jump button release
        if (isHoldingJump && (!Input.GetKey(KeyCode.W) && !Input.GetKey(KeyCode.UpArrow) && !Input.GetKey(KeyCode.Space)))
        {
            isHoldingJump = false;
            rb.gravityScale = originalGravityScale;
        }
    }

    private void Jump()
    {
        // Initial jump
        if (jumpPressed)
        {
            rb.velocity = new Vector2(rb.velocity.x, jumpForce);
            jumpPressed = false;
        }
        
        // addintional jump force in the air
        if (isHoldingJump && !isGrounded && jumpHoldTime <= maxJumpHoldTime && !isHeadHit)
        {
            // additional force up
            rb.AddForce(Vector2.up * additionalJumpForce * Time.fixedDeltaTime, ForceMode2D.Force);
            
            // less gravity while jumping
            rb.gravityScale = jumpHoldGravityScale;
        }
        else if (!isHoldingJump || isGrounded)
        {
            // reset gravity
            if (!isHeadHit)
            {
                rb.gravityScale = originalGravityScale;
            }
        }
    }

    #region VELOCITY_DEBUG - Remove this region when done testing
    void OnGUI()
    {
        if (showVelocityOnScreen)
        {
            GUIStyle style = new GUIStyle();
            style.fontSize = 20;
            style.normal.textColor = Color.white;
            
            GUI.Label(new Rect(10, 10, 300, 30), $"Velocity Y: {rb.velocity.y:F2}", style);
            GUI.Label(new Rect(10, 40, 300, 30), $"Velocity Magnitude: {rb.velocity.magnitude:F2}", style);
            GUI.Label(new Rect(10, 70, 300, 30), $"Is Grounded: {isGrounded}", style);
            GUI.Label(new Rect(10, 100, 300, 30), $"Is Head Hit: {isHeadHit}", style);
        }
    }
    #endregion

    void OnDrawGizmos()
    {
        if (groundCheck != null)
        {
            Gizmos.color = isGrounded ? Color.green : Color.red;
            Gizmos.DrawWireSphere(groundCheck.position, groundCheckRadius);
        }
        
        if (headCheck != null)
        {
            bool headCollision = Physics2D.OverlapCircle(headCheck.position, headCheckRadius, obstacleLayer);
            Gizmos.color = headCollision ? Color.yellow : Color.blue;
            Gizmos.DrawWireSphere(headCheck.position, headCheckRadius);
        }
    }
}
