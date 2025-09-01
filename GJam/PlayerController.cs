using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class PlayerController : MonoBehaviour
{
    [Header("Audio Settings")]
    private AudioSource moveAudioSource;
    public AudioClip moveClip;

    [Header("Footprint Settings")]
    public GameObject footprintPrefab;
    private Vector2 previousPosition;

    [Header("Movement Settings")]
    public float moveSpeed = 5f;
    public float gridSize = 1f;

    [Header("References")]
    public LayerMask wallLayer = 1;
    public LayerMask boxLayer = 1;
    public float pushDelay = 0.02f;
    public CameraZoom cameraZoom; // Ссылка на CameraZoom для проверки состояния зума
    public PlayerHealth playerHealth; // Ссылка на PlayerHealth для получения урона

    private Vector2 targetPosition;
    private bool isMoving = false;
    private GridSystem gridSystem;
    
    void Start(){
        if (moveAudioSource == null)
            moveAudioSource = GetComponent<AudioSource>();
        previousPosition = transform.position;
    
        targetPosition = transform.position;
        gridSystem = FindObjectOfType<GridSystem>();
        
        if (gridSystem == null)
        {
            Debug.LogError("GridSystem not found! Please add GridSystem to your scene.");
        }
        
        // Если CameraZoom не назначен, попробуем найти его автоматически
        if (cameraZoom == null)
        {
            cameraZoom = FindObjectOfType<CameraZoom>();
            if (cameraZoom == null)
            {
                Debug.LogWarning("CameraZoom not found! Player input will not be blocked during zoom.");
            }
        }
        
        // Если PlayerHealth не назначен, попробуем найти его автоматически
        if (playerHealth == null)
        {
            playerHealth = GetComponent<PlayerHealth>();
        }
    }
    
    void Update()
    {
        HandleInput();
        MoveToTarget();
        
    }
    
    void HandleInput()
    {
        // Блокируем ввод, если игрок движется, камера зумится или ввод отключен глобально
        if (isMoving || IsCameraZooming() || LevelTriggerZone.isInputDisabled) return;
        
        Vector2 input = Vector2.zero;
        
        if (Input.GetKeyDown(KeyCode.W) || Input.GetKeyDown(KeyCode.UpArrow))
        {
            input = Vector2.up;
        }
        else if (Input.GetKeyDown(KeyCode.S) || Input.GetKeyDown(KeyCode.DownArrow))
        {
            input = Vector2.down;
        }
        else if (Input.GetKeyDown(KeyCode.A) || Input.GetKeyDown(KeyCode.LeftArrow))
        {
            input = Vector2.left;
            // Flip to face left
            Vector3 scale = transform.localScale;
            scale.x = Mathf.Abs(scale.x) * -1f;
            transform.localScale = scale;
        }
        else if (Input.GetKeyDown(KeyCode.D) || Input.GetKeyDown(KeyCode.RightArrow))
        {
            input = Vector2.right;
            // Flip to face right
            Vector3 scale = transform.localScale;
            scale.x = Mathf.Abs(scale.x);
            transform.localScale = scale;
        }
        
        if (input != Vector2.zero)
        {
            TryMove(input);
        }
    }
    
    // Метод для проверки, зумится ли камера
    bool IsCameraZooming()
    {
        if (cameraZoom != null)
        {
            return cameraZoom.IsZooming();
        }
        return false;
    }
    
    void TryMove(Vector2 direction)
    {
        Vector2 newPosition = targetPosition + direction * gridSize;
        
        // Проверяем, можно ли двигаться в эту позицию
        if (CanMoveTo(newPosition, direction))
        {
            targetPosition = newPosition;
            isMoving = true;
        }
    }
    
    bool CanMoveTo(Vector2 position, Vector2 direction)
    {
        // Проверяем стены с улучшенной системой коллизий
        if (CollisionHelper.CheckCollisionAtPosition(position, wallLayer))
        {
            return false;
        }
        
        // Проверяем коробки
        Collider2D boxCollider = Physics2D.OverlapPoint(position, boxLayer);
        if (boxCollider != null)
        {
            // Пытаемся толкнуть коробку
            Box box = boxCollider.GetComponent<Box>();
            if (box != null && !box.IsMoving())
            {
                Vector2 boxCurrentPosition = box.transform.position;
                
                // Проверяем, можно ли толкнуть коробку
                if (CollisionHelper.CanPushBox(boxCurrentPosition, direction, gridSize, wallLayer, boxLayer))
                {
                    // Синхронно толкаем коробку и двигаем игрока
                    StartCoroutine(PushBoxAndMove(box, direction));
                    return false; // Возвращаем false, так как движение обрабатывается в корутине
                }
                else
                {
                    return false;
                }
            }
            else
            {
                return false; // Коробка движется или не найдена
            }
        }
        
        return true;
    }
    

    
    void MoveToTarget()
    {
        if (isMoving && Vector2.Distance(transform.position, targetPosition) > 0.01f)
        {
            Vector2 newPosition = Vector2.MoveTowards(transform.position, targetPosition, moveSpeed * Time.deltaTime);
            transform.position = new Vector3(newPosition.x, newPosition.y, transform.position.z);
        }
        else if (isMoving)
        {
            
            // Принудительно устанавливаем позицию и завершаем движение
            transform.position = new Vector3(targetPosition.x, targetPosition.y, transform.position.z);

            // Спавним footprintPrefab на предыдущей клетке
            if (footprintPrefab != null)
            {
                Debug.Log($"[PlayerController] === spawning footprint at: {previousPosition}");
                Instantiate(footprintPrefab, previousPosition, Quaternion.identity);
            }

            // Проигрываем звук движения
            if (moveAudioSource != null && moveClip != null)
            {
                moveAudioSource.PlayOneShot(moveClip);
                Debug.Log($"[PlayerController] === PLaying move sound: {moveClip.name}");
            }

            // Обновляем previousPosition
            previousPosition = transform.position;
            Debug.Log($"[PlayerController]  {previousPosition}, {targetPosition}, {transform.position}");
            isMoving = false;

            // Вызываем TakeDamage() после завершения движения
            playerHealth?.TakeDamage();
        }
    }
    
    // Метод для получения текущей позиции в гриде
    public Vector2Int GetGridPosition()
    {
        if (gridSystem != null)
        {
            return gridSystem.WorldToGridPosition(transform.position);
        }
        return Vector2Int.zero;
    }
    
    // Корутина для синхронного толкания коробки и движения игрока
    IEnumerator PushBoxAndMove(Box box, Vector2 direction)
    {
        isMoving = true;
        
        // Толкаем коробку
        box.Push(direction, gridSize);
        
        // Ждем небольшой момент для синхронизации
        yield return new WaitForSeconds(pushDelay);
        
        // Двигаем игрока
        targetPosition = transform.position + (Vector3)(direction * gridSize);
        
        // Ждем завершения движения
        while (Vector2.Distance(transform.position, targetPosition) > 0.01f)
        {
            Vector2 newPosition = Vector2.MoveTowards(transform.position, targetPosition, moveSpeed * Time.deltaTime);
            transform.position = new Vector3(newPosition.x, newPosition.y, transform.position.z);
            yield return null;
        }
        
        transform.position = new Vector3(targetPosition.x, targetPosition.y, transform.position.z);
        isMoving = false;
        Instantiate(footprintPrefab, previousPosition, Quaternion.identity);
        previousPosition = transform.position;
        // Вызываем TakeDamage() после завершения движения с коробкой
        playerHealth?.TakeDamage();
    }
}
