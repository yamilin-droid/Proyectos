import pygame
import random
import sys
import time

# Inicializar Pygame
pygame.init()

# Constantes
WIDTH, HEIGHT = 800, 600
GRID_SIZE = 20
GRID_WIDTH = WIDTH // GRID_SIZE
GRID_HEIGHT = HEIGHT // GRID_SIZE
INITIAL_FPS = 10
FPS_INCREASE = 2  # How much FPS increases per level
MAX_FPS = 25  # Maximum FPS to prevent the game from becoming too fast
INITIAL_LEVEL_TARGET = 3  # Points needed to complete first level

# Colores
BLACK = (0, 0, 0)
GREEN = (0, 255, 0)
RED = (255, 0, 0)
WHITE = (255, 255, 255)

# Direcciones
UP = (0, -1)
DOWN = (0, 1)
LEFT = (-1, 0)
RIGHT = (1, 0)

class Snake:
    def __init__(self):
        start_x = GRID_WIDTH // 2
        start_y = GRID_HEIGHT // 2
        # Inicializar la serpiente con 5 segmentos en línea horizontal
        self.positions = [(start_x - i, start_y) for i in range(5)]
        self.direction = RIGHT
        self.length = 5
        self.score = 0

    def get_head_position(self):
        return self.positions[0]

    def update(self):
        cur = self.get_head_position()
        x, y = self.direction
        new = ((cur[0] + x) % GRID_WIDTH, (cur[1] + y) % GRID_HEIGHT)
        
        if new in self.positions[1:]:
            return False  # Game over
        
        self.positions.insert(0, new)
        if len(self.positions) > self.length:
            self.positions.pop()
        return True

    def reset(self):
        start_x = GRID_WIDTH // 2
        start_y = GRID_HEIGHT // 2
        self.positions = [(start_x - i, start_y) for i in range(5)]
        self.direction = RIGHT
        self.length = 5
        self.score = 0

    def render(self, surface):
        for p in self.positions:
            rect = pygame.Rect(p[0] * GRID_SIZE, p[1] * GRID_SIZE, GRID_SIZE, GRID_SIZE)
            pygame.draw.rect(surface, GREEN, rect)
            pygame.draw.rect(surface, BLACK, rect, 1)

class Food:
    def __init__(self):
        self.position = (0, 0)
        self.randomize_position()

    def randomize_position(self, avoid_positions=None):
        if avoid_positions is None:
            avoid_positions = []
        while True:
            self.position = (random.randint(0, GRID_WIDTH - 1), 
                           random.randint(0, GRID_HEIGHT - 1))
            if self.position not in avoid_positions:
                break

    def render(self, surface):
        rect = pygame.Rect(self.position[0] * GRID_SIZE, 
                          self.position[1] * GRID_SIZE, 
                          GRID_SIZE, GRID_SIZE)
        pygame.draw.rect(surface, RED, rect)
        pygame.draw.rect(surface, BLACK, rect, 1)

class Trap:
    def __init__(self, permanent=False):
        self.position = (0, 0)
        self.active = False  # Start inactive
        self.permanent = permanent
        self.spawn_time = 0
        self.duration = float('inf') if permanent else 5  # Permanent traps last forever

    def spawn(self, avoid_positions):
        self.position = (random.randint(0, GRID_WIDTH - 1), 
                        random.randint(0, GRID_HEIGHT - 1))
        while self.position in avoid_positions:
            self.position = (random.randint(0, GRID_WIDTH - 1), 
                           random.randint(0, GRID_HEIGHT - 1))
        self.active = True
        self.spawn_time = time.time()

    def update(self):
        if self.active and (time.time() - self.spawn_time > self.duration):
            self.active = False
            return True  # Indicates trap expired
        return False

    def render(self, surface):
        if self.active:
            rect = pygame.Rect(self.position[0] * GRID_SIZE,
                             self.position[1] * GRID_SIZE,
                             GRID_SIZE, GRID_SIZE)
            color = WHITE  # Todas las trampas son blancas
            pygame.draw.rect(surface, color, rect)
            # Mostrar temporizador solo para trampas no permanentes
            if not self.permanent:
                time_left = max(0, int(self.duration - (time.time() - self.spawn_time)))
                font = pygame.font.SysFont('Arial', 12)
                text = font.render(str(time_left), True, BLACK)  # Texto en negro para mejor visibilidad
                text_rect = text.get_rect(center=rect.center)
                surface.blit(text, text_rect)

def add_trap(snake, food, traps):
    trap = Trap(permanent=False)
    avoid_positions = snake.positions + [food.position] + [t.position for t in traps if t.active]
    trap.spawn(avoid_positions)
    traps.append(trap)
    return True

def main():
    # Configuración de la ventana
    screen = pygame.display.set_mode((WIDTH, HEIGHT))
    pygame.display.set_caption('Snake Game')
    clock = pygame.time.Clock()
    font = pygame.font.SysFont('Arial', 25)
    small_font = pygame.font.SysFont('Arial', 20)

    snake = Snake()
    food = Food()
    # Crear lista de trampas (las añadiremos dinámicamente)
    traps = []
    
    game_over = False
    level = 1
    current_fps = INITIAL_FPS
    score = 0
    level_target = INITIAL_LEVEL_TARGET
    last_trap_spawn = 0  # Seguimiento de cuándo se generó la última trampa
    trap_spawn_interval = 3  # Segundos entre generación de trampas
    max_traps = 5  # Número máximo de trampas que pueden estar activas a la vez

    # Posicionar trampas y comida
    def reset_game_state():
        nonlocal score, level_target, level, current_fps
        snake.reset()
        food.randomize_position()
        
        # Reiniciar estado del juego
        score = 0
        level = 1
        level_target = INITIAL_LEVEL_TARGET
        current_fps = INITIAL_FPS
        return False  # game_over = False
    
    game_over = reset_game_state()
    add_trap(snake, food, traps)
    last_trap_spawn = time.time()

    while True:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                pygame.quit()
                sys.exit()
            elif event.type == pygame.KEYDOWN:
                if game_over:
                    if event.key == pygame.K_r:
                        traps.clear()  # Limpiar trampas al reiniciar
                        game_over = reset_game_state()
                        add_trap(snake, food, traps)
                        last_trap_spawn = time.time()
                else:
                    if event.key == pygame.K_UP and snake.direction != DOWN:
                        snake.direction = UP
                    elif event.key == pygame.K_DOWN and snake.direction != UP:
                        snake.direction = DOWN
                    elif event.key == pygame.K_LEFT and snake.direction != RIGHT:
                        snake.direction = LEFT
                    elif event.key == pygame.K_RIGHT and snake.direction != LEFT:
                        snake.direction = RIGHT

        if not game_over:
            # Actualizar serpiente
            if not snake.update():
                game_over = True
                continue

            head_pos = snake.get_head_position()
            
            # Comprobar colisión con trampas
            for trap in traps:
                if trap.active and head_pos == trap.position:
                    # Encontrar el segmento que chocó con la trampa
                    for i, pos in enumerate(snake.positions):
                        if pos == head_pos:
                            # Eliminar el segmento que chocó con la trampa
                            snake.positions = snake.positions[:i] + snake.positions[i+1:]
                            snake.length = len(snake.positions)
                            
                            # Si no quedan segmentos, juego terminado
                            if snake.length == 0:
                                game_over = True
                                break
                                
                            # Solo las trampas temporales desaparecen al colisionar
                            if not trap.permanent:
                                trap.active = False
                            break
                    break
            
            # Comprobar si la serpiente come la comida
            if head_pos == food.position:
                snake.length += 1
                score += 1
                
                # Verificar si sube de nivel
                if score >= level_target:
                    level += 1
                    level_target *= 2  # Duplicar el objetivo para el siguiente nivel
                    current_fps = min(INITIAL_FPS + (level - 1) * FPS_INCREASE, MAX_FPS)
                    # Limpiar trampas existentes y añadir una inicial para el nuevo nivel
                    traps.clear()
                    add_trap(snake, food, traps)
                    last_trap_spawn = time.time()
                    
                    # Reposicionar comida
                    avoid = snake.positions + [t.position for t in traps]
                    food.randomize_position(avoid)
                
                # Obtener nueva posición de la comida (evitando la serpiente y trampas activas)
                avoid_positions = snake.positions.copy()
                avoid_positions.extend(t.position for t in traps if t.active)
                food.randomize_position(avoid_positions)
            
            # Generar nueva trampa cada pocos segundos si estamos por debajo del máximo de trampas
            current_time = time.time()
            if (current_time - last_trap_spawn >= trap_spawn_interval and 
                len(traps) < max_traps):
                if add_trap(snake, food, traps):
                    last_trap_spawn = current_time
            
            # Actualizar todas las trampas
            for trap in traps[:]:  # Crear una copia para eliminación segura
                if trap.update() and not trap.active:
                    traps.remove(trap)

        # Dibujar
        screen.fill(BLACK)
        snake.render(screen)
        food.render(screen)
        # Renderizar todas las trampas activas
        for trap in traps:
            if trap.active:
                trap.render(screen)

        # Mostrar información
        score_text = font.render(f'Puntuación: {score}', True, WHITE)
        level_text = font.render(f'Nivel: {level}', True, WHITE)
        target_text = small_font.render(f'Objetivo: {score}/{level_target}', True, WHITE)
        
        screen.blit(score_text, (10, 10))
        screen.blit(level_text, (10, 40))
        screen.blit(target_text, (10, 70))

        if game_over:
            game_over_text = font.render('¡Juego Terminado! Presiona R para reiniciar', True, WHITE)
            text_rect = game_over_text.get_rect(center=(WIDTH//2, HEIGHT//2))
            screen.blit(game_over_text, text_rect)

        pygame.display.flip()
        clock.tick(current_fps)

if __name__ == "__main__":
    main()